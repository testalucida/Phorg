//#include <iostream>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Enumerations.H>
#include <FL/names.h>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <fltk_ext/Canvas.h>
#include <fltk_ext/DragBox.h>
#include <fltk_ext/TextMeasure.h>

#include "../images/open.xpm"

#include "std.h"

#include <my/StringHelper.h>

#include <dirent.h>
#include <stdexcept>

using namespace std;

class Box: public DragBox {
public:
	Box( int x, int y, int w, int h ) : DragBox( x, y, w, h ), _X(x) {
		box( FL_UP_BOX );
		color( FL_LIGHT2 );
	}

	void resize( int x, int y, int w, int h ) {
		Fl_Box::resize( x, y, w, h );
		fprintf( stderr, "Box::resize: %d, %d, %d, %d\n", x, y, w, h );
		image()->scale( round( float(w) * _zoom ), round( (float)h * _zoom ), 1, 1 );
//		float rel = (float)image()->data_w() / (float)image()->data_h();
//		fprintf( stderr, "data_w: %d, data_h: %d\n",
//				image()->data_w(), image()->data_h() );
//		float draw_h = w / rel;
////		fprintf( stderr, "rel: %f, draw_h: %f\n", rel, draw_h );
//		int W = round( w * _zoom );
//		int H = round( draw_h * _zoom );
////		float W_H = (float)W / (float)H;
////		fprintf( stderr, "scaling: W = %d, H = %d, W_H = %f\n", W, H, W_H );
//		image()->scale( W, H, 1, 1 );
	}

	void setZoom( float zoom ) {
		_zoom = zoom;
		resize( x(), y(), w(), h() );
	}

	int handle( int evt ) {
		int rc = DragBox::handle( evt );
		//const char* evtname = fl_eventnames[evt];
		//fprintf( stderr, "evt: %d -- %s\n", evt, evtname );
		switch( evt ) {
		case FL_MOUSEWHEEL: { //FL_MOUSEWHEEL - 19
			if( Fl::belowmouse() == this ) {
				fprintf( stderr, "dx: %d, dy: %d\n", Fl::event_dx(), Fl::event_dy() );
				_zoom += ( 0.2 * Fl::event_dy() );
				_zoom = ( _zoom < 0.2 ) ? 0.2 : _zoom;
				int W = round( (float)w() * _zoom );
				int H = round( (float)h() * _zoom );
				image()->scale( W, H, 1, 1 );
				resize( x(), y(), image()->w(), image()->h() );
				parent()->redraw();
			}
			return 1;
		}
		default:
			return rc;
		}
	}
protected:
	void draw() {
		DragBox::draw();
	}
	void draw_custom() {}
private:
	float _zoom = 1;
	int _X;
}; //class Box

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const long MAX_MEM_USAGE = 800000000;
//const long MAX_MEM_USAGE = 100000000;
//+++++++++++++++++    PHOTOBOX   +++++++++++++++++++++++++++
class PhotoBox : public Fl_Box {
public:
	PhotoBox( int x, int y, int w, int h,
			const char* folder = NULL, const char* file = NULL ) :
		Fl_Box( x, y, w, h )
	{
		if( folder ) _folder.append( folder );
		if( file ) {
			_file.append( file );
			_filename_size =
					TextMeasure::inst().get_size( file, _font, _fontsize );
		}

		box( FL_FLAT_BOX );
		color( FL_LIGHT1 );
	}

	~PhotoBox() {
		Fl_Image* img;
		if( ( img = image() ) != NULL ) {
			delete img;
		}
	}

	void resize( int x, int y, int w, int h ) {
		Fl_Box::resize( x, y, w, h );
		Fl_Image* img = image();
		if( img ) {
			image()->scale( this->w(), this->h(), 1, 1 );
		}
	}

	void setSelected( bool selected ) {
		_isSelected = selected;
		redraw();
	}

	void getPhotoPathnFile( string& pathnfile ) const {
		pathnfile = _folder;
		pathnfile.append( "/" );
		pathnfile.append( _file );
	}

//	int handle( int evt ) {
//		int rc = Fl_Box::handle( evt );
//		switch( evt ) {
//		case FL_PUSH:
//			_isSelected = true;
//			redraw();
//			rc = 1;
//			break;
//		default:
//			break;
//		}
//
//		return rc;
//	}

protected:
	void draw() {
		fl_push_clip( x(), y(), w(), h() );
		Fl_Box::draw();
		//draw filename below picture
		fl_color( FL_BLACK );
		int xc = x() + w()/2 - _filename_size.w/2;
		fl_draw( _file.c_str(), xc, y() + h() - 12 );
		//draw selection border if box is selected
		if( _isSelected ) {
			int margin = 2;
			fl_line_style( FL_SOLID, 3 );
			fl_color( FL_YELLOW );
			float x1 = x();
			float x2 = x() + w() - margin;
			float y1 = y();
			float y2 = y() + h() - margin;
			fl_begin_line();
			fl_vertex( x1, y1 );
			fl_vertex( x2, y1 );
			fl_vertex( x2, y2 );
			fl_vertex( x1, y2 );
			fl_vertex( x1, y1 );
			fl_end_line();

			fl_line_style( 0 );
		}
		fl_pop_clip();
	}
private:
	string _folder;
	string _file;
	Fl_Font _font = FL_HELVETICA;
	Fl_Fontsize _fontsize = 12;
	Size _filename_size;
	bool _isSelected = false;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++    SCROLL    +++++++++++++++++++++++++++
typedef void (*ResizeCallback) ( int x, int y, int w, int h, void* data );
typedef void (*ScrollCallback) ( int xpos, int ypos, void* data );

class Scroll : public Fl_Scroll {
public:
	Scroll( int x, int y, int w, int h ) : Fl_Scroll( x, y, w, h ) {
		box( FL_FLAT_BOX );
		color( FL_BLACK );
		end();
	}

	void setResizeCallback( ResizeCallback cb, void* data ) {
		_resize_cb = cb;
		_resize_cb_data = data;
	}

	void setScrollCallback( ScrollCallback cb, void* data ) {
		_scroll_cb = cb;
		_scroll_cb_data = data;
	}

	void resize( int x, int y, int w, int h ) {
		Fl_Scroll::resize( x, y, w, h );
		if( _resize_cb ) {
			(_resize_cb)( x, y, w, h, _resize_cb_data );
		}
	}

	int handle( int evt ) {
		int rc = Fl_Scroll::handle( evt );
		//fprintf( stderr, "Scroll::handle -- evt = %d, %s\n", evt, fl_eventnames[evt] );
		switch( evt ) {
		case FL_PUSH: {
			unselectAll();
			Fl_Widget* w = Fl::belowmouse();
			PhotoBox* box = dynamic_cast<PhotoBox*>( w );
			if( box ) {
				box->setSelected( true );
			}
			_pushed = true;
			return 1;
		}
		case FL_MOVE:
			if( _pushed ) {
				doScrollCallback();
				_pushed = false;
			}
			break;
		case FL_LEAVE:
		case FL_RELEASE:
		case FL_MOUSEWHEEL:
			doScrollCallback();
			break;
		default:
			break;
		}

		return rc;
	}

	void doScrollCallback() {
		//fprintf( stderr, "y(): %d, yposition(): %d\n", y(), yposition() );
		int ypos = yposition();
		int xpos = xposition();
		if( _scroll_cb && ( ypos != _ypos_old || xpos != _xpos_old ) ) {
			(_scroll_cb)( xpos, ypos, _scroll_cb_data );
			_xpos_old = xpos;
			_ypos_old = ypos;
		}

	}

	void unselectAll() {
		for( int i = 0, imax = children(); i < imax; i++ ) {
			PhotoBox* box = dynamic_cast<PhotoBox*>( child( i ) );
			if( box ) {
				box->setSelected( false );
			}
		}
	}

private:
	ResizeCallback _resize_cb = NULL;
	void* _resize_cb_data = NULL;
	ScrollCallback _scroll_cb = NULL;
	void* _scroll_cb_data = NULL;
	bool _pushed = false;
	int _ypos_old = 0, _xpos_old = 0;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++  TOOLBAR  +++++++++++++++++++++++++
static const char* SYMBOL_LAST = "@>|";
static const char* SYMBOL_NEXT = "@>";
static const char* SYMBOL_PREVIOUS = "@<";
static const char* SYMBOL_FIRST = "@|<";

class ToolBar : public Fl_Group {
public:
	ToolBar( int x, int y, int w ) : Fl_Group( x, y, w, 34 ) {
		//type( Fl_Pack::HORIZONTAL );    // horizontal packing of buttons
		box( FL_FLAT_BOX );
		color( FL_DARK2 );
		//spacing( 4 );            // spacing between buttons
		end();
	}

	/**
	 * Adds a button to the right side of the most right button on the left side of the toolbar.
	 */
	void addButton( const char** xpm, const char *tooltip = 0,
			        Fl_Callback *cb=0, void *data=0 )
	{
		Position xy = getXYfromLeft();
		Fl_Button* b = createButton( xy.x, xy.y, tooltip, cb, data );

		Fl_Pixmap* img = new Fl_Pixmap( xpm );
		b->image( img );
		_mostRightLeftButton = b;
	}

	void addButton( const char* symbol, Fl_Align align, const char *tooltip = 0,
				    Fl_Callback *cb=0, void *data=0 )
	{
		Position xy;
		if( align == FL_ALIGN_LEFT ) {
			xy = getXYfromLeft();
		} else if( align == FL_ALIGN_RIGHT ) {
			xy = getXYfromRight();
		} else {
			throw runtime_error( "ToolBar::addButton: FL_ALIGN_CENTER not yet implemented" );
		}
		Fl_Button* btn = createButton( xy.x, xy.y, tooltip, cb, data );
		btn->copy_label( symbol );

		if( align == FL_ALIGN_LEFT ) {
			_mostRightLeftButton = btn;
		} else { //FL_ALIGN_RIGHT
			_mostLeftRightButton = btn;
		}
	}

	void setPageButtonEnabled( const char* buttonsymbol, bool enable ) {
		for( int i = 0, imax = children(); i < imax; i++ ) {
			Fl_Button* btn = dynamic_cast<Fl_Button*>( child(i) );
			if( btn ) {
				const char* label = btn->label();
				if( label ) {
					if( !strcmp( label, buttonsymbol ) ) {
						if( enable ) {
							btn->activate();
						} else {
							btn->deactivate();
						}
					}
				} //if( label )
			} //if( btn )
		} //for
	}

	void setAllPageButtonsEnabled( bool enable ) {
		for ( int i = 0, imax = children(); i < imax; i++ ) {
			Fl_Button *btn = dynamic_cast<Fl_Button*>( child( i ) );
			if ( btn ) {
				const char *label = btn->label();
				if ( label ) {
					if ( label[0] == '@') {
						if ( enable ) {
							btn->activate();
						} else {
							btn->deactivate();
						}
					}
				} //if( label )
			} //if( btn )
		} //for
	}

private:
	Fl_Button* createButton( int x, int y, const char *tooltip = 0,
			Fl_Callback *cb = 0, void *data = 0 )
	{
		begin();
		Fl_Button *b = new Fl_Button( x, y, _buttonsize, _buttonsize );
		b->box( FL_FLAT_BOX );    // buttons won't have 'edges'
		b->color( FL_DARK2 );
		b->clear_visible_focus();

		if ( tooltip )
			b->tooltip( tooltip );

		if ( cb )
			b->callback( cb, data );

		end();
		return b;
	}

	Position getXYfromLeft() const {
		Position pos;
		pos.x = _mostRightLeftButton ? _mostRightLeftButton->x() + _mostRightLeftButton->w() + _spacing_x
				                     : _spacing_x;
		pos.y = 1;
		return pos;
	}

	Position getXYfromRight() const {
		Position pos;
		pos.x = _mostLeftRightButton ? _mostLeftRightButton ->x() - _spacing_x - _buttonsize
							         : x() + w() - _spacing_x - _buttonsize;
		pos.y = 1;
		return pos;
	}
private:
	Fl_Button* _mostRightLeftButton = NULL;
	Fl_Button* _mostLeftRightButton = NULL;
	int _spacing_x = 4;
	int _buttonsize = 32;
};
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//*+++++++++++++++++++++++  CONTROLLER  +++++++++++++++++++++++
class Controller {
private:
	struct PhotoInfo {
		std::string folder;
		std::string filename;
		PhotoBox* box = NULL;
	};

	enum Page {
		NEXT,
		PREVIOUS,
		LAST,
		FIRST
	};

public:
	Controller( Scroll* scroll, ToolBar* toolbar ) : _scroll(scroll), _toolbar(toolbar) {
		_scroll->setResizeCallback( onCanvasResize_static, this );
		_scroll->setScrollCallback( onScrolled_static, this );
		_toolbar->setAllPageButtonsEnabled( false );
	}

	~Controller() {
		deletePhotoInfos();
	}

	static void onCanvasResize_static( int x, int y, int w, int h, void* data ) {
		//Controller* pThis = (Controller*)data;
		//pThis->layoutPhotos( x, y, w, h );
	}

	static void onScrolled_static( int xpos, int ypos, void* data ) {
//		Controller* pThis = (Controller*)data;
//		pThis->checkLoadPhotosAfterScrolling( xpos, ypos );
	}

	static void onOpen_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->openFolder();
	}

	static void onChangePage( Fl_Widget* btn, void* data ) {
		Controller* pThis = (Controller*)data;
		string label;
		label.append( btn->label() );
		if( label == SYMBOL_NEXT ) {
			pThis->browseNextPage();
		} else if( label == SYMBOL_LAST ) {
			pThis->browseLastPage();
		} else if( label == SYMBOL_PREVIOUS ) {
			pThis->browsePreviousPage();
		} else {
			pThis->browseFirstPage();
		}
	}

	void openFolder() {
		Fl_Native_File_Chooser native;
		native.title( "Choose folder with photos to show" );
		//native.directory(G_filename->value());
		native.type( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
		// Show native chooser
		switch ( native.show() ) {
		case -1: /*ERROR -- todo*/
			;
			break; // ERROR
		case 1: /*CANCEL*/
			;
			fl_beep();
			break; // CANCEL
		default: // PICKED DIR
			if ( native.filename() ) {
				string title = "Photo Organization";
				title.append( ": " );
				title.append( native.filename() );
				((Fl_Double_Window*)_scroll->parent())->label( title.c_str() );
				/*get photos from selected dictionary*/
				readPhotos( native.filename() );
			} else {
				/* do nothing */
			}
			break;
		}
	}

	void readPhotos( const char* folder ) {
		DIR *dir;
		struct dirent *ent;
		if ( (dir = opendir( folder )) != NULL ) {
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
			deletePhotoInfos();
			/* print all the files and directories within directory */
			while ( (ent = readdir( dir )) != NULL ) {
				//fprintf( stderr, "%s\n", ent->d_name );
				if( ent->d_type == DT_REG && ent->d_name[0] != '.' ) {
					if( isImageFile( ent->d_name ) ) {
						addImageFile( folder, ent->d_name );
					}
				}
			}
			closedir( dir );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
			layoutPhotos( Page::FIRST );
		} else {
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
			/* could not open directory */
			string msg = "Couldn't open folder ";
			msg.append( folder );
			throw runtime_error( msg );
		}
	}

	bool isImageFile( const char* filename ) {
		my::StringHelper& strh = my::StringHelper::instance();
		if( strh.endsWith( filename, "jpg" ) ||
			strh.endsWith( filename, "png" ) ||
			strh.endsWith( filename, "jpeg" ) ||
			strh.endsWith( filename, "JPG" ) ||
			strh.endsWith( filename, "JPEG " ) ||
			strh.endsWith( filename, "PNG" ) )
		{
			return true;
		}

		return false;
	}

	void addImageFile( const char* folder, const char* filename ) {
		//todo: delete PhotoInfos in controller's destructor
		PhotoInfo* pinfo = new PhotoInfo;
		pinfo->folder.append( folder );
		pinfo->filename.append( filename );
		_photos.push_back( pinfo );
	}

	void layoutPhotos( Page page ) {
		//remove and destroy all previous PhotoBoxes
		removePhotosFromCanvas();
		_usedBytes = 0;

		int X = _scroll->x() + _spacing_x;
		int Y = _scroll->y() + _spacing_y;
		int n = 0;  //number of photos per row
		int rows = 0;
		//calculate photo index to start and to end with
		setStartAndEndIndexes( page );

		auto itr = _photos.begin() + _photoIndexStart;
		int idx = _photoIndexStart;
		for(  ; itr != _photos.end() && idx <= _photoIndexEnd; itr++, idx++ ) {
			if( n > 2 ) { //start next row
				if( ++rows > 3 ) break;
				X = _scroll->x() + _spacing_x;
				Y += ( _box_h + _spacing_y );
				n = 0;
			}
			PhotoInfo* pinfo = *itr;
			if( pinfo->box == NULL ) {
				if( _usedBytes > MAX_MEM_USAGE ) break;

				pinfo->box = new PhotoBox( X, Y, _box_w, _box_h,
						                  pinfo->folder.c_str(),
										  pinfo->filename.c_str() );
				loadPhoto( pinfo->box );
			}
			_scroll->add( pinfo->box );
			X += ( _box_w + _spacing_x );
			n++;
		} //for
		adjustPageButtons();
		_scroll->redraw();
	}

	/** Remove and destroy all photos from canvas (scroll area). */
	void removePhotosFromCanvas() {
		_scroll->clear();
		for( auto pinfo : _photos ) {
			pinfo->box = NULL;
		}
	}

	void setStartAndEndIndexes( Page page ) {
		switch( page ) {
		case Page::FIRST:
			_photoIndexStart = 0;
			_photoIndexEnd = _photosPerRow * _maxRows - 1;
			validateEndIndex();
			break;
		case Page::NEXT:
			_photoIndexStart = _photoIndexEnd + 1;
			if( _photoIndexStart >= (int)_photos.size() ) {
				//start from the beginning
				_photoIndexStart = 0;
			}
			_photoIndexEnd = _photoIndexStart + _photosPerRow * _maxRows - 1;
			validateEndIndex();
			break;
		case Page::PREVIOUS:
			_photoIndexEnd = _photoIndexStart - 1;
			_photoIndexStart = _photoIndexEnd - ( _photosPerRow * _maxRows - 1 );
			validateStartIndex();
			break;
		case Page::LAST:
			_photoIndexEnd = (int)_photos.size();
			_photoIndexStart = _photoIndexEnd - ( _photosPerRow * _maxRows - 1 );
			validateStartIndex();
			break;
		}

	}

	inline void validateEndIndex() {
		if( _photoIndexEnd >= (int)_photos.size() ) {
			_photoIndexEnd = _photos.size() - 1;
		}
	}

	inline void validateStartIndex() {
		_photoIndexStart = _photoIndexStart < 0 ? 0 : _photoIndexStart;
	}

private:
	void loadPhoto( PhotoBox* box ) {
		string pathnfile;
		box->getPhotoPathnFile( pathnfile );
		Fl_JPEG_Image* jpg = new Fl_JPEG_Image( pathnfile.c_str() );
		jpg->scale( _box_w, _box_h, 1, 1 );
		_usedBytes += getPhotoSize( jpg );
		box->image( jpg );
	}

	inline long getPhotoSize( Fl_Image* img ) {
		return ( img->data_w() * img->data_h() * img->d() ) ;
	}

	void browseNextPage() {
		layoutPhotos( Page::NEXT );
	}

	void browseLastPage() {
		layoutPhotos( Page::LAST );
	}

	void browsePreviousPage() {
		layoutPhotos( Page::PREVIOUS );
	}

	void browseFirstPage() {
		layoutPhotos( Page::FIRST );
	}

	void deletePhotoInfos() {
		for( auto pinfo : _photos ) {
			delete pinfo;
		}
		_photos.clear();
	}

	void adjustPageButtons() {
		_toolbar->setAllPageButtonsEnabled( false );
		if( _photoIndexStart > 0 ) {
			_toolbar->setPageButtonEnabled( SYMBOL_FIRST, true );
			_toolbar->setPageButtonEnabled( SYMBOL_PREVIOUS, true );
		}
		if( _photoIndexEnd < _photos.size() - 1 ) {
			_toolbar->setPageButtonEnabled( SYMBOL_NEXT, true );
			_toolbar->setPageButtonEnabled( SYMBOL_LAST, true );
		}
	}

private:
	Scroll* _scroll;
	ToolBar* _toolbar;
	int _box_w = 400;
	int _box_h = 400;
	int _spacing_x = 10;
	int _spacing_y = 5;

	int _photosPerRow = 3;
	int _maxRows = 4;

	std::vector<PhotoInfo*> _photos;
	int _photoIndexStart = -1;
	int _photoIndexEnd = -1;
	long _usedBytes = 0;
};
//*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * Application to get organization into your photos' folders.
 * View photo, delete it or move or copy it into another folder quickly.
 * Sort them in a conveniant way based on their names or date of taking.
 */

int main() {
	// additional linker options: -lfltk_images -ljpeg -lpng
	fl_register_images();
	Fl_Double_Window *win =
			new Fl_Double_Window(200, 200, 500, 500, "Photo Organization" );

	ToolBar* tb = new ToolBar( 0, 0, 500 );
	Scroll* scroll = new Scroll( 0, tb->h(), 500, win->h() - tb->h() );

	Controller ctrl( scroll, tb );
	tb->addButton( open_xpm, "Open photo folder...", Controller::onOpen_static, &ctrl );
	tb->addButton( SYMBOL_LAST, FL_ALIGN_RIGHT, "Browse last page", Controller::onChangePage, &ctrl );
	tb->addButton( SYMBOL_NEXT, FL_ALIGN_RIGHT, "Browse next page", Controller::onChangePage, &ctrl );
	tb->addButton( SYMBOL_PREVIOUS, FL_ALIGN_RIGHT, "Browse previous page", Controller::onChangePage, &ctrl );
	tb->addButton( SYMBOL_FIRST, FL_ALIGN_RIGHT, "Browse first page", Controller::onChangePage, &ctrl );

	win->resizable( scroll );
	win->end();
	win->show();

	return Fl::run();
}

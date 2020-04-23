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
#include <FL/Fl_SVG_Image.H>
#include <FL/fl_ask.H>
#include <fltk_ext/Canvas.h>
#include <fltk_ext/DragBox.h>
#include <fltk_ext/TextMeasure.h>

#include "../images/open.xpm"
#include "../images/manage_folders.xpm"

#include "std.h"
#include "const.h"
#include "FolderManager.hpp"
#include "FolderDialog.hpp"

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
			const char* folder = NULL, const char* file = NULL, const char* datetime = NULL ) :
		Fl_Box( x, y, w, h )
	{
		if( folder ) _folder.append( folder );
		if( file ) {
			_file.append( file );
			_filename_size =
					TextMeasure::inst().get_size( file, _font, _fontsize );
		}
		if( datetime ) {
			_datetime.append( datetime );
			_datetime_size = TextMeasure::inst().get_size( datetime, _font, _fontsize );
		}

		box( FL_FLAT_BOX );
		color( FL_LIGHT1 );
	}

	~PhotoBox() {
		//fprintf( stderr, "delete PhotoBox %s with its image.\n", _file.c_str() );
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

		//draw the buttons above picture
		//first draw a rectangle to write in hence when a
		//picture is displayed portrait the buttons cannot be seen.
		int xc = x() + w()/2;
		//we have 3 buttons (garbage/dunno/good) each of which is 30 px
		//with a spacing of 4 px between.

		//draw the text below picture: name of photo and time taken.
		int xcf = xc - _filename_size.w/2;
		int xcd = xc - _datetime_size.w/2;
		int Y = y() + h() - 20;
		//first draw a rectangle to write in hence when a
		//picture is drawn portrait the text is hardly to see.
		int X = xcf < xcd ? xcf : xcd;
		int W = _filename_size.w > _datetime_size.w ? _filename_size.w
				                                    : _datetime_size.w;
		fl_rectf( X, Y-15, W+20 /*dunno why*/, 35, FL_LIGHT2 );

		//draw filename below picture
		fl_color( FL_BLACK );
		fl_draw( _file.c_str(), xcf, Y );

		//draw datetime below filename
		Y += 16;
		fl_draw( _datetime.c_str(), xcd, Y );

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
	string _datetime;
	Fl_Font _font = FL_HELVETICA;
	Fl_Fontsize _fontsize = 12;
	Size _filename_size;
	Size _datetime_size;
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

enum ToolId {
	OPEN_FOLDER,
	MANAGE_FOLDERS,
	RENAME_FILES,
	LAST_PAGE,
	NEXT_PAGE,
	PREVIOUS_PAGE,
	FIRST_PAGE,
	NONE
};

struct Tool {
	ToolId toolId = ToolId::NONE;
	Fl_Button* btn = NULL;
};

class ToolBar : public Fl_Group {
public:
	ToolBar( int x, int y, int w ) : Fl_Group( x, y, w, 34 ) {
		//type( Fl_Pack::HORIZONTAL );    // horizontal packing of buttons
		box( FL_FLAT_BOX );
		color( FL_DARK2 );
		//spacing( 4 );            // spacing between buttons
		end();
	}

	~ToolBar() {
		for( auto t : _tools ) {
			delete t;
		}
	}

	/**
	 * Adds a button to the right side of the most right button on the left side of the toolbar.
	 */
	void addButton( ToolId id, const char** xpm, const char *tooltip = 0,
			        Fl_Callback *cb=0, void *data=0 )
	{
		Position xy = getXYfromLeft();
		Fl_Button* b = createButton( id, xy.x, xy.y, tooltip, cb, data );

		Fl_Pixmap* img = new Fl_Pixmap( xpm );
		b->image( img );
		_mostRightLeftButton = b;
	}

	void addButton( ToolId id, const char* symbol, Fl_Align align, const char *tooltip = 0,
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

		Fl_Button* btn = createButton( id, xy.x, xy.y, tooltip, cb, data );
		if( symbol ) btn->copy_label( symbol );

		if( align == FL_ALIGN_LEFT ) {
			_mostRightLeftButton = btn;
		} else { //FL_ALIGN_RIGHT
			_mostLeftRightButton = btn;
		}
	}

	void addButton( ToolId id, const char* svg_file, const char *tooltip = 0,
				    Fl_Callback *cb=0, void *data=0 )
	{
		addButton( id, NULL, FL_ALIGN_LEFT, tooltip, cb, data );
		const Tool& tool = getTool( id );
		Fl_Button* btn = tool.btn;
		Fl_SVG_Image *svg = new Fl_SVG_Image( svg_file );
		svg->scale( btn->w() - 2, btn->h() - 2 );
		btn->image( svg );
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

	void setManageFoldersButtonEnabled( bool enable ) {
		const Tool& t = getTool( ToolId::MANAGE_FOLDERS );
		if( enable ) {
			t.btn->activate();
		} else {
			t.btn->deactivate();
		}
	}

	void setRenameFilesButtonEnabled( bool enable ) {
		const Tool& t = getTool( ToolId::RENAME_FILES );
		if( enable ) {
			t.btn->activate();
		} else {
			t.btn->deactivate();
		}
	}

	/** inserts a resizable dummy boy which prevents buttons from
	 * resizing when the application window is resized.
	 * Call this method after having added all toolbar buttons.
	 */
	void fixButtonsOnResize() {
		if( _filler == NULL )  {
			int X = x() + w()/2;
			_filler = new Fl_Box( X, y(), 1, 1 );
			_filler->box( FL_FLAT_BOX );
			//_filler->color( FL_RED );
			add( _filler );
			resizable( _filler );
		} else {

		}
	}

	const Tool& getTool( ToolId id ) {
		for( auto t : _tools ) {
			if( t->toolId == id ) {
				return *t;
			}
		}
		throw runtime_error( "ToolBar::getTool(): Tool " + to_string( id ) + " not found." );
	}

private:
	Fl_Button* createButton( ToolId id, int x, int y, const char *tooltip = 0,
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
		_tools.push_back( new Tool {id, b} );

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
	Fl_Box* _filler = NULL;
	int _spacing_x = 4;
	int _buttonsize = 32;
	vector<Tool*> _tools;
};
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//*+++++++++++++++++++++++  CONTROLLER  +++++++++++++++++++++++
class Controller {
private:
	struct PhotoInfo {
		std::string folder;
		std::string filename;
		std::string datetime;
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
		_scroll->scrollbar.linesize( 80 );
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

	static void onManageFolders_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->showFolderDialog();
	}

	static void onRenameFiles_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->renameFiles();
	}

	static void onChangePage_static( Fl_Widget* btn, void* data ) {
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
		const char* folder = _folderManager.chooseFolder();
		if( folder ) {
			string title = "Photo Organization";
			title.append( ": " );
			title.append( folder );
			((Fl_Double_Window*)_scroll->parent())->label( title.c_str() );
			/*get photos from selected dictionary*/
			readPhotos( folder );
			_toolbar->setManageFoldersButtonEnabled( true );
			_toolbar->setRenameFilesButtonEnabled( true );

			_folder.clear();
			_folder.append( folder );
		}
	}

	void renameFiles() {
		int resp = fl_choice( "Rename all photos?\n"
				              "Resulting filenames will be of format\nYYYYMMDD_TTTTTT.jpg",
							  "No", "Yes", NULL );
		if( resp == 1 ) {
			_folderManager.renameFilesToDatetime( _folder.c_str() );
			readPhotos( _folder.c_str() );
		}
	}

	void readPhotos( const char* folder ) {
		reset();
		vector<ImageInfo*>& imagefiles = _folderManager.getImages( folder );
		for( auto img : imagefiles ) {
			addImageFile( img->folder.c_str(), img->filename.c_str(), img->datetime.c_str() );
		}

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
		layoutPhotos( Page::FIRST );
	}

	void reset() {
		removePhotosFromCanvas();
		//_photos.clear();
		deletePhotoInfos();
		_usedBytes = 0;
		_photoIndexStart = -1;
		_photoIndexEnd = -1;
	}

	void addImageFile( const char* folder, const char* filename, const char* datetime ) {
		PhotoInfo* pinfo = new PhotoInfo;
		pinfo->folder.append( folder );
		pinfo->filename.append( filename );
		pinfo->datetime.append( datetime );
		_photos.push_back( pinfo );
	}

	void layoutPhotos( Page page ) {
//		fprintf( stderr, "*********** clocking start in layoutPhotos ************\n" );
//		my::Timer timer;

		//remove and destroy all previous PhotoBoxes
//		timer.start();
		removePhotosFromCanvas();
//		timer.stop();
//		fprintf( stderr, "time needed for removePhotosFromCanvas: %s\n",
//						         timer.durationToString());
		_usedBytes = 0;

		int X = _scroll->x() + _spacing_x;
		int Y = _scroll->y() + _spacing_y;
		int n = 0;  //number of photos per row
		int rows = 0;
		//calculate photo index to start and to end with
		setStartAndEndIndexes( page );

//		timer.start();
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
										  pinfo->filename.c_str(),
										  pinfo->datetime.c_str() );
				loadPhoto( pinfo->box );
			}
			_scroll->add( pinfo->box );
			X += ( _box_w + _spacing_x );
			n++;
		} //for
//		timer.stop();
//		fprintf( stderr, "time needed for for loop creating PhotoBoxes: %s\n",
//						         timer.durationToString());
		adjustPageButtons();
		_scroll->redraw();
//		fprintf( stderr, "*********** layoutPhotos: clocking end ************\n" );
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

	void showFolderDialog() {
		FolderDialog* dlg = new FolderDialog( 100, 100, _folder.c_str() );
		dlg->setCreateFolderCallback( FolderManager::onCreateFolders,
				                      &_folderManager );
		string folder = _folder;
		folder.append( "/" );
		string checkfolder = GARBAGE_FOLDER;
		if( _folderManager.existsFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::GARBAGE, false );
		}
		checkfolder = GOOD_FOLDER;
		if( _folderManager.existsFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::GOOD, false );
		}
		checkfolder = DUNNO_FOLDER;
		if( _folderManager.existsFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::DUNNO, false );
		}

		dlg->show( true );
		delete dlg;
	}


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
		if( _photoIndexEnd < (int)_photos.size() - 1 ) {
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

	std::vector<PhotoInfo*> _photos; //contains all photos of current folder.
	int _photoIndexStart = -1;
	int _photoIndexEnd = -1;
	long _usedBytes = 0;
	FolderManager _folderManager;
	string _folder; //current folder whose photos are displayed
};
//*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "sys/types.h"
#include "sys/sysinfo.h"

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int g_used_kb = 0;
long long g_physmem_used = 0;

int getUsedMemory() { //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
//        if (strncmp(line, "VmSize:", 7) == 0){ //Virtual Memory currently used by current process
    	if (strncmp(line, "VmRSS:", 6) == 0) { //Physical Memory currently used by current process
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

void checkMemory() {
	g_used_kb = getUsedMemory();
	struct sysinfo memInfo;
	sysinfo (&memInfo);
	long long totalVirtualMem = memInfo.totalram;
	//Add other values in next statement to avoid int overflow on right hand side...
	totalVirtualMem += memInfo.totalswap;
	totalVirtualMem *= memInfo.mem_unit;
	long long physMemUsed = memInfo.totalram - memInfo.freeram;
	//Multiply in next statement to avoid int overflow on right hand side...
	physMemUsed *= memInfo.mem_unit;
	g_physmem_used = physMemUsed;
}

/**
 * Application to get organization into your photos' folders.
 * View photo, delete it or move or copy it into another folder quickly.
 * Sort them in a conveniant way based on their names or date of taking.
 */
int main() {
	checkMemory();
	// additional linker options: -lfltk_images -ljpeg -lpng
	fl_register_images();
	Fl_Double_Window *win =
			new Fl_Double_Window(200, 200, 500, 500, "Photo Organization" );

	ToolBar* tb = new ToolBar( 0, 0, 500 );
	Scroll* scroll = new Scroll( 0, tb->h(), 500, win->h() - tb->h() );
	Controller ctrl( scroll, tb );
	tb->addButton( ToolId::OPEN_FOLDER, open_xpm, "Choose photo folder", Controller::onOpen_static, &ctrl );
	tb->addButton( ToolId::MANAGE_FOLDERS, manage_folders_xpm, "List, create and delete subfolders", Controller::onManageFolders_static, &ctrl );
	tb->addButton( ToolId::LAST_PAGE, SYMBOL_LAST, FL_ALIGN_RIGHT, "Browse last page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::NEXT_PAGE, SYMBOL_NEXT, FL_ALIGN_RIGHT, "Browse next page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::PREVIOUS_PAGE, SYMBOL_PREVIOUS, FL_ALIGN_RIGHT, "Browse previous page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::FIRST_PAGE, SYMBOL_FIRST, FL_ALIGN_RIGHT, "Browse first page", Controller::onChangePage_static, &ctrl );
	const char* f = "/home/martin/Projects/cpp/Phorg/images/rename_files.svg";
	tb->addButton( ToolId::RENAME_FILES, f, "Rename all jpg files in folder", Controller::onRenameFiles_static, &ctrl );
	tb->fixButtonsOnResize();
	tb->setAllPageButtonsEnabled( false );
	tb->setManageFoldersButtonEnabled( false );
	tb->setRenameFilesButtonEnabled( false );
	win->resizable( scroll );
	win->end();
	win->show();

	int rc = Fl::run();
	int used_kb = g_used_kb;
	long long physmem_used = g_physmem_used;
	checkMemory();
	fprintf( stderr, "*************End of program***************\n" );
	fprintf( stderr, "Delta used_kb: %d\n", g_used_kb - used_kb );
	fprintf( stderr, "Delta physmem_used: %lld \n", g_physmem_used - physmem_used );
	return rc;
}

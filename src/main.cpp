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
		const char* evtname = fl_eventnames[evt];
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
//const long MAX_MEM_USAGE = 1000000000;
const long MAX_MEM_USAGE = 100000000;
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
		fprintf( stderr, "y(): %d, yposition(): %d\n", y(), yposition() );
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
class ToolBar : public Fl_Group {
public:
	ToolBar( int x, int y, int w ) : Fl_Group( x, y, w, 34 ) {
		//type( Fl_Pack::HORIZONTAL );    // horizontal packing of buttons
		box( FL_FLAT_BOX );
		color( FL_DARK2 );
		//spacing( 4 );            // spacing between buttons
		end();
	}

	void addButton( const char** xpm = 0, const char *tooltip = 0,
			       Fl_Callback *cb=0, void *data=0 )
	{
		begin();
		Fl_Button *b = new Fl_Button( 4, 1, 32, 32 );
		b->box( FL_FLAT_BOX );    // buttons won't have 'edges'
		b->color( FL_DARK2 );
		b->clear_visible_focus();

		if( xpm ) {
			Fl_Pixmap* img = new Fl_Pixmap( xpm );
			b->image( img );
		}

		if ( tooltip )
			b->tooltip( tooltip );

		if ( cb )
			b->callback( cb, data );

		end();
	}
private:
};
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//*+++++++++++++++++++++++  CONTROLLER  +++++++++++++++++++++++
class Controller {
private:
	struct PhotoInfo {
		PhotoBox* box = NULL;
	};
public:
	Controller( Scroll* scroll ) : _scroll(scroll) {
		_scroll->setResizeCallback( onCanvasResize_static, this );
		_scroll->setScrollCallback( onScrolled_static, this );
	}

	static void onCanvasResize_static( int x, int y, int w, int h, void* data ) {
		Controller* pThis = (Controller*)data;
		pThis->layoutPhotos( x, y, w, h );
	}

	static void onScrolled_static( int xpos, int ypos, void* data ) {
		Controller* pThis = (Controller*)data;
		pThis->checkLoadPhotosAfterScrolling( xpos, ypos );
	}

	static void onOpen_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->onOpen();
	}

	void onOpen() {
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
				getPhotos( native.filename() );
			} else {
				/* do nothing */
			}
			break;
		}
	}

	void getPhotos( const char* folder ) {
		DIR *dir;
		struct dirent *ent;
		if ( (dir = opendir( folder )) != NULL ) {
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
			/* print all the files and directories within directory */
			while ( (ent = readdir( dir )) != NULL ) {
				//fprintf( stderr, "%s\n", ent->d_name );
				if( ent->d_type == DT_REG && ent->d_name[0] != '.' ) {
					addPhoto( folder, ent->d_name );
				}
			}
			closedir( dir );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
			layoutPhotos( _scroll->x(), _scroll->y(), _scroll->w(), _scroll->h() );
		} else {
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
			/* could not open directory */
			string msg = "Couldn't open folder ";
			msg.append( folder );
			throw runtime_error( msg );
		}
	}

	void addPhoto( const char* folder, const char* filename ) {
		PhotoBox* box = new PhotoBox( 1, 1, _box_w, _box_h, folder, filename );

		if( _usedBytes < MAX_MEM_USAGE ) {
			loadPhoto( box );
		}
		_photos.push_back( box );
	}

	void layoutPhotos( int x, int y, int w, int h ) {
		for( int i = 0, imax = _scroll->children(); i < imax; i++ ) {
			PhotoBox* box = dynamic_cast<PhotoBox*>( _scroll->child(0) );
			if( box ) {
				_scroll->remove(0);
			}
		}
		int X = x + _spacing_x;
		int Y = y + _spacing_y;
		for( auto ph : _photos ) {
			if( X > ( x + w ) ) {
				X = x + _spacing_x;
				Y += ( _box_h + _spacing_y );
			}
			_scroll->add( ph );
			ph->position( X, Y );
			X += ( _box_w + _spacing_x );
		}
		_scroll->redraw();
	}

	/**
	 * By scrolling a row of PhotoBoxes might come in view whose images are not loaded.
	 * We have to unload the PhotoBoxes in an upper row and then load the boxes of the
	 * row coming in sight.
	 * @params xpos, ypos: scroll amount. E.g. ypos = 16 means that Fl_Scroll is extending
	 * the northern border for 16 pixels.
	 */
	void checkLoadPhotosAfterScrolling( int xpos, int ypos ) {
		//check the bottommost y value we can see:
		int ymax = _scroll->y() + _scroll->h() + ypos;
		//get the PhotoBoxes containing ymax and load them if necessary:
		int row_y = 0;
		vector<PhotoBox*> to_load;
		for( auto ph : _photos ) {
			if( ph->y() < ymax && ( ph->y() + ph->h() ) >= ymax ) {
				if( row_y == 0 ) row_y = ph->y();
				if( row_y != ph->y() ) break;
				if( !ph->image() ) {
					to_load.push_back( ph );
				}
			}
		}

		if( to_load.size() > 0 ) {
			//we have photos to reload, so unload the topmost
			//row of PhotoBoxes with loaded photos
			row_y = 0;
			for( auto ph : _photos ) {
				Fl_Image* img;
				if( ( img = ph->image() ) != NULL ) {
					_usedBytes -= getPhotoSize( img );
					delete img;
					ph->image( NULL );
				}
			}
		}

		//...and load photos contained in vector to_load:
		for( auto ph : to_load ) {
			loadPhoto( ph );
		}

		_scroll->redraw();
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

private:
	Scroll* _scroll;
	int _box_w = 400;
	int _box_h = 400;
	int _spacing_x = 10;
	int _spacing_y = 5;
	//PhotoBox* _lastBox = NULL;
	std::vector<PhotoBox*> _photos;
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

	Controller ctrl( scroll );
	tb->addButton( open_xpm, "Open photo folder...", Controller::onOpen_static, &ctrl );

	//ctrl.addPicture( "/home/martin/Bilder/2020/03/IMG_3956.JPG" );
//	Fl_JPEG_Image jpg("/home/martin/Bilder/2020/03/IMG_3956.JPG");
//	int w = 200, h = 200;
//	jpg.scale( w, h, 1, 1 );

	win->resizable( scroll );
	win->end();
	win->show();

	return Fl::run();
}

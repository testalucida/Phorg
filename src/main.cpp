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
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>
#include <fltk_ext/Canvas.h>
#include <fltk_ext/DragBox.h>
#include <fltk_ext/TextMeasure.h>
#include <fltk_ext/FlxDialog.h>
#include <fltk_ext/FlxButton.h>
#include <fltk_ext/FlxStatusBox.h>

#include "../images/open.xpm"
#include "../images/manage_folders.xpm"
//#include "../images/move_files.xpm"

#include "std.h"
#include "const.h"
#include "global.h"
#include "FolderManager.hpp"
#include "FolderDialog.hpp"

#include <my/StringHelper.h>

#include <dirent.h>
#include <stdexcept>

using namespace std;

class TestImage : public Fl_JPEG_Image {
public:
	TestImage( const char* file ) : Fl_JPEG_Image( file ) {
	}

	void draw( int x, int y ) {
		fprintf( stderr, "TestImage::draw(): x = %d, y = %d\n", x, y );
		Fl_JPEG_Image::draw( x,y );
	}

};

class Box: public Fl_Box {
public:
	Box( int x, int y, int w, int h ) :
		Fl_Box( x, y, w, h ), _img_x( x ), _img_y( y )
	{
		box( FL_FLAT_BOX );
		color( FL_LIGHT2 );
		//fprintf( stderr, "constructor: img_x, img_y: %d, %d\n", _img_x, _img_y );
	}

	~Box() {
		Fl_Image* img;
		if( ( img = image() ) != NULL ) {
			delete img;
		}
	}

	int handle( int evt ) {
		static int dx = 0;
		static int dy = 0;
		static int wheel = 0;
		int rc = Fl_Box::handle( evt );
		//const char* evtname = fl_eventnames[evt];
		//fprintf( stderr, "evt: %d -- %s\n", evt, evtname );
		switch( evt ) {
		case FL_PUSH:
			dx = Fl::event_x();
			dy = Fl::event_y();
			if( _zoom > 1.0 )  {
				this->top_window()->cursor( FL_CURSOR_MOVE );
				Fl::check();
//				float zoomfactor = _zoom - 1;
//				_img_x = _img_x - round( ( (float)_img_x * zoomfactor ) );
//				_img_y = _img_y - round( ( (float)_img_y * zoomfactor ) );
			}
			return 1;
		case FL_RELEASE:
			fl_cursor( FL_CURSOR_DEFAULT );
			dx = Fl::event_x();
			dy = Fl::event_y();
			return 1;
		case FL_DRAG: {
			if( _zoom > 1.0 ) {
				dx -= Fl::event_x(); //gt 0: dragged to the left
				dy -= Fl::event_y(); //gt 0: dragged to the top
				_img_x -= dx;
				_img_y -= dy;
				Fl_Image* img = this->image();
				///////////// WORK IN PROGRESS  ///////////////////////
				//img->draw( _img_x, _img_y );
				int offx = x() - _img_x;
				int offy = y() - _img_y;
				img->draw( x(), y(), w(), h(), offx, offy );
				//////////////////////////////////////////////////////////////////
//				fprintf( stderr, "dx = %d, dy = %d -- drew image at %d, %d\n",
//						 dx, dy, _img_x, _img_y );
				dx = Fl::event_x();
				dy = Fl::event_y();

			}
			return 1;
		}
		case FL_MOUSEWHEEL: { //FL_MOUSEWHEEL - 19
			if( Fl::belowmouse() == this && Fl::event_key( FL_Control_L ) ) {
				Fl_Image* img = image();
				_zoom += ( 0.2 * Fl::event_dy() );
				_zoom = ( _zoom < 0.2 ) ? 0.2 : _zoom;
				int oldw = img->w();
				int oldh = img->h();
				if( wheel == 0 ) { //first Mousewheel event
					//adjust _img_y:
					int dh = h() - oldh;
					_img_y += dh/2;

					//adjust _img_x:
					int dw = w() - oldw;
					_img_x += dw/2;
					wheel = 1;
				}
				int W = round( (float)w() * _zoom );
				int H = round( (float)h() * _zoom );
				img->scale( W, H, 1, 1 );
				int neww = img->w();
				int newh = img->h();
				int dw = oldw - neww;
				int dh = oldh - newh;
				_img_x += dw/2;
				_img_y += dh/2;
//				fprintf( stderr, "oldw, oldh: %d, %d, neww, newh: %d, %d, img_x, img_y: %d, %d\n",
//						         oldw, oldh, neww, newh, _img_x, _img_y );
				//resize( x(), y(), image()->w(), image()->h() );
				parent()->redraw();
				return 1;
			}
			return rc;
		}
		default:
			return rc;
		}
	}


//	void resize( int x, int y, int w, int h ) {
//		Fl_Box::resize( x, y, w, h );
//		Fl_Image* img = image();
//		if( img ) {
//			image()->scale( this->w(), this->h(), 1, 1 );
//		}
//	}

protected:
	void draw() {
		fl_push_clip( x(), y(), w(), h() );
		Fl_Box::draw();
		fl_pop_clip();
	}

private:
	float _zoom = 1;
	int _img_x;
	int _img_y;

}; //class Box

class ImageFactory {
public:
	static ImageFactory& inst() {
		static ImageFactory* pThis = NULL;
		if( !pThis ) {
			pThis = new ImageFactory();
		}
		return *pThis;
	}

	Fl_SVG_Image* getRed() const {
		return _svg_red;
	}
	Fl_SVG_Image* getYellow() const {
		return _svg_red;
	}
	Fl_SVG_Image* getGreen() const {
		return _svg_red;
	}

	Fl_SVG_Image* getPaleRed() const {
		return _svg_pale_red;
	}
	Fl_SVG_Image* getPaleYellow() const {
		return _svg_pale_yellow;
	}
	Fl_SVG_Image* getPaleGreen() const {
		return _svg_pale_green;
	}
	Fl_SVG_Image* getBurger() const {
		return _svg_burger;
	}
private:
	ImageFactory() {
		_svg_red = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/red.svg" );
		_svg_yellow = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/yellow.svg" );
		_svg_green = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/green.svg" );
		_svg_pale_red = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/pale_red.svg" );
		_svg_pale_yellow = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/pale_yellow.svg" );
		_svg_pale_green = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/pale_green.svg" );
		_svg_burger = new Fl_SVG_Image( "/home/martin/Projects/cpp/Phorg/images/burger.svg" );
	}

private:
	Fl_SVG_Image* _svg_red = NULL;
	Fl_SVG_Image* _svg_yellow = NULL;
	Fl_SVG_Image* _svg_green = NULL;
	Fl_SVG_Image* _svg_pale_red = NULL;
	Fl_SVG_Image* _svg_pale_yellow = NULL;
	Fl_SVG_Image* _svg_pale_green = NULL;
	Fl_SVG_Image* _svg_burger = NULL;
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const long MAX_MEM_USAGE = 800000000;
//const long MAX_MEM_USAGE = 100000000;
//+++++++++++++++++    PHOTOBOX   +++++++++++++++++++++++++++
class PhotoBox : public Fl_Group { // Fl_Box {
public:
	PhotoBox( int x, int y, int w, int h,
			const char* folder = NULL, const char* file = NULL, const char* datetime = NULL ) :
		Fl_Group( x, y, w, h )
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
		color( FL_LIGHT2 );

		int btn_w = 30;
		int btn_h = 30;
		_box = new Box( x, y + btn_h, w, h - btn_h );

		int spacing_x = 5;
		int x1 = this->x() + 10;
		_btnRed = newToggleButton( x1, y + 1, btn_w, btn_h,
				                   ImageFactory::inst().getPaleRed(), FL_RED );
		_btnRed->tooltip( "Earmark this photo for disposal (move into garbage folder)." );
		x1 += btn_w + spacing_x;
		_btnYellow = newToggleButton( x1, y + 1, btn_w, btn_h,
				                      ImageFactory::inst().getPaleYellow(), FL_YELLOW );
		_btnYellow->tooltip( "Earmark this photo for moving it into the dunno folder." );
		x1 += btn_w + spacing_x;
		_btnGreen = newToggleButton( x1, y + 1, btn_w, btn_h,
				                     ImageFactory::inst().getPaleGreen(), FL_GREEN );
		_btnGreen->tooltip( "Earmark this photo for moving it into the 'good' folder." );

		//A centered group between the 3 color buttons and the burger button.
		//Containing label "move to: " and a box
		//containing the name of the folder the photo is to move to.
		int label_w = 40; //width of "move to: " label
		int box_w = 150; //width of the box with folder name
		int xc = this->x() + this->w()/2;
		int grpx = xc - (label_w + spacing_x + box_w)/2 + 2*spacing_x;
		int grpw = label_w + box_w + 3*spacing_x;
		int spacing_y = 4;
		Fl_Group* moveto_grp = new Fl_Group( grpx, y + spacing_y, grpw, 25 );

		//the label box
		Fl_Box* moveto_lbl = new Fl_Box( grpx, y + spacing_y, label_w, 25, "move to: " );
		moveto_lbl->box( FL_NO_BOX );
		moveto_lbl->labelsize( 10 );
		//the folder name box
		int boxx = grpx + label_w + spacing_x;
		_moveto = new Fl_Box( boxx, y + spacing_y, box_w, 25);
		_moveto->box( FL_FLAT_BOX );
		_moveto->color( FL_LIGHT1 );
		_moveto->labelsize( 11 );
		moveto_grp->end();

		x1 = this->x() + this->w() - btn_w - spacing_x;
		_btnBurger = new Fl_Button( x1, y + 1, btn_w, btn_h );
		_btnBurger->box( FL_FLAT_BOX );
		_btnBurger->color( FL_LIGHT2 );
		_btnBurger->clear_visible_focus();
		Fl_SVG_Image* img = ImageFactory::inst().getBurger();
		img->scale( btn_w, btn_h );
		_btnBurger->image( img );
		_btnBurger->tooltip( "Choose folder by menu to move this photo into." );
		_btnBurger->callback( onBurgerMenu_static, this );

		end();

	}

	~PhotoBox() {
	}

	Size getPhotoSize() const {
		Size size = { _box->w(), _box->h() };
		return size;
	}

	void image( Fl_Image* img ) {
		_box->image( img );
	}

	Fl_Image* image() {
		return _box->image();
	}


	void setSelected( bool selected ) {
		_isSelected = selected;
		redraw();
	}

	bool isSelected() const {
		return _isSelected;
	}

	void setRedButton( bool pressed ) {
		_btnRed->value( pressed ? 1 : 0 );
		_moveto->label( GARBAGE_FOLDER );
	}

	void setYellowButton( bool pressed ) {
		_btnYellow->value( pressed ? 1 : 0 );
	}

	void setGreenButton( bool pressed ) {
		_btnGreen->value( pressed ? 1 : 0 );
	}

	Fl_Color getSelectedColor() const {
		if( _btnRed->value() != 0 ) return FL_RED;
		if( _btnYellow->value() != 0 ) return FL_YELLOW;
		if( _btnGreen->value() != 0 ) return FL_GREEN;
		return 0;
	}

	void setSelectedColor( Fl_Color color ) {
		syncFolderNameAndColor( color );
	}

	const char* getMoveToFolder() const  {
		return _moveto->label();
	}

	/**
	 * Writes text into the moveto Fl_Box.
	 * Doesn't make any synchronizations with ToggleButtons.
	 */
	void setMoveToFolder( const char* folder ) {
		_moveto->copy_label( folder );
	}

	void unsetExcept( Fl_Toggle_Button* btn ) {
		if ( btn == _btnRed ) {
			setYellowButton( false );
			setGreenButton( false );
		} else if ( btn == _btnYellow ) {
			setRedButton( false );
			setGreenButton( false );
		} else if ( btn == _btnGreen ) {
			setRedButton( false );
			setYellowButton( false );
		}
	}

	const string& getPhotoPathnFile() const {
		_pathnfile = _folder;
		_pathnfile.append( "/" );
		_pathnfile.append( _file );
		return _pathnfile;
	}

	Box* getBox() const {
		return _box;
	}

//	int handle( int e ) {
//		if( e == FL_PUSH && Fl::event_button3() ) {
//			fprintf( stderr, "PhotoBox::handle( FL_PUSH )\n" );
//			return 1;
//		}
//		return Fl_Group::handle( e );
//	}

protected:
	void draw() {
		fl_push_clip( x(), y(), w(), h() );
		Fl_Group::draw();

		int xc = x() + w()/2;
		drawBottomBox( xc );

		if( _isSelected ) {
			drawSelectionBorder();
		}
		fl_pop_clip();
	}


	/**
	 * Draw a rectangle below photo to write into hence when a
	 * picture is drawn portrait the text is hardly to see.
	 * Draw text below picture: name of photo and time taken.
	 * @param xc: x coordinate of the center of this box.
	 */
	inline void drawBottomBox( int xc ) {
		int xcf = xc - _filename_size.w/2;
		int xcd = xc - _datetime_size.w/2;
		int Y = y() + h() - 20;

		int X = xcf < xcd ? xcf : xcd;
		int W = _filename_size.w > _datetime_size.w ? _filename_size.w
													: _datetime_size.w;
		fl_rectf( X, Y-15, W+20 /*dunno why*/, 35, FL_LIGHT2 );

		//draw filename
		fl_color( FL_BLACK );
		fl_draw( _file.c_str(), xcf, Y );

		//draw datetime below filename
		Y += 16;
		fl_draw( _datetime.c_str(), xcd, Y );
	}

	inline void drawSelectionBorder() {
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

private:
	Fl_Toggle_Button* newToggleButton( int x, int y, int w, int h,
								       Fl_Image* img, Fl_Color down_color )
	{
		Fl_Toggle_Button* btn = new Fl_Toggle_Button( x, y, w, h );
		btn->box( FL_FLAT_BOX );
		btn->color( FL_LIGHT2 );
		btn->clear_visible_focus();
		img->scale( w, h, 1, 1 );
		btn->image( img );
		btn->down_color( down_color );
		btn->callback( onToggled_static, this );
		return btn;
	}

	static void onToggled_static( Fl_Widget* w, void* data ) {
		PhotoBox* box = (PhotoBox*)data;
		box->onToggled( (Fl_Toggle_Button*)w );
	}

	static void onBurgerMenu_static( Fl_Widget* w, void* data ) {
		PhotoBox* box = (PhotoBox*)data;
		box->showBurgerMenu( (Fl_Button*)w );
	}

	void onToggled( Fl_Toggle_Button* btn ) {
		bool on = ( btn->value() != 0 );
		if( on ) {
			unsetExcept( btn );
			syncFolderNameAndToggleButton( btn );
		} else {
			_moveto->label( "" );
		}
	}

	void showBurgerMenu( Fl_Button* burger ) {
		vector<string> folders;
		FolderManager& fm = FolderManager::inst();
		//fm.getFolders( fm.getCurrentFolder().c_str(), folders );
		fm.getFolders( "/home/martin/Projects/cpp/Phorg/testphotos", folders );
		Fl_Menu_Item menu[folders.size()+1];
		for( int i = 0, imax = folders.size(); i < imax; i++ ) {
			menu[i] = { folders.at(i).c_str() };
		}
		menu[folders.size()] = { 0 };

		//get length of longest menu item
		int len = getWidestLen( folders, _box->labelfont(), _box->labelsize() );
		const Fl_Menu_Item *m = menu->popup( burger->x() - len,
				                             burger->y()+burger->h(),
											 0, 0, 0 );
		if( m ) {
			//write folder name into moveto box
			_tmp_moveto.clear();
			_tmp_moveto.append( m->text );
			syncFolderNameAndToggleButton( _tmp_moveto.c_str() );
		}
	}

	int getWidestLen( const vector<string>& items, Fl_Font font, Fl_Fontsize fontsize ) {
		TextMeasure& tm = TextMeasure::inst();
		int len = 0;
		for( auto item : items ) {
			Size sz = tm.get_size( item.c_str(), font, fontsize );
			len = sz.w > len ? sz.w : len;
		}
		return len;
	}

    void syncFolderNameAndToggleButton( const char* foldernameToSet ) {
    	setRedButton( false );
    	setYellowButton( false );
    	setGreenButton( false );
    	_moveto->label( foldernameToSet );
    	if( !strcmp( GARBAGE_FOLDER, foldernameToSet ) ) {
    		setRedButton( true );
    	} else if( !strcmp( DUNNO_FOLDER, foldernameToSet ) ) {
    		setYellowButton( true );
    	} else if( !strcmp( GOOD_FOLDER, foldernameToSet ) ) {
    		setGreenButton( true );
    	}
    }

    void syncFolderNameAndToggleButton( const Fl_Toggle_Button* btnSelected ) {
		if( btnSelected == _btnRed ) {
			_moveto->label( GARBAGE_FOLDER );
		} else if( btnSelected == _btnYellow ) {
			_moveto->label( DUNNO_FOLDER );
		} else if( btnSelected == _btnGreen ) {
			_moveto->label( GOOD_FOLDER );
		}
	}

    void syncFolderNameAndColor( Fl_Color colorToSet ) {
    	string moveto;
		switch( colorToSet ) {
		case FL_RED:
			setRedButton( true );
			unsetExcept( _btnRed );
			moveto = GARBAGE_FOLDER;
			break;
		case FL_YELLOW:
			setYellowButton( true );
			unsetExcept( _btnYellow );
			moveto = DUNNO_FOLDER;
			break;
		case FL_GREEN:
			setGreenButton( true );
			unsetExcept( _btnGreen );
			moveto = GOOD_FOLDER;
			break;
		default:
			setRedButton( false );
			setYellowButton( false );
			setGreenButton( false );
			break;
		}
		_moveto->copy_label( moveto.c_str() );
	}

private:
	Fl_Toggle_Button* _btnRed = NULL;
	Fl_Toggle_Button* _btnYellow = NULL;
	Fl_Toggle_Button* _btnGreen = NULL;
	Fl_Box* _moveto = NULL;
	mutable string _tmp_moveto;
	Fl_Button* _btnBurger = NULL;
	string _folder;
	string _file;
	mutable string _pathnfile;
	string _datetime;
	Fl_Font _font = FL_HELVETICA;
	Fl_Fontsize _fontsize = 12;
	Size _filename_size;
	Size _datetime_size;
	bool _isSelected = false;
	int _topbox_h = 32;
	Fl_SVG_Image* _svgRed = NULL;
	Box* _box = NULL;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++    SCROLL    +++++++++++++++++++++++++++
typedef void (*ResizeCallback) ( int x, int y, int w, int h, void* data );
typedef void (*ScrollCallback) ( int xpos, int ypos, void* data );
typedef void (*ClickCallback)  ( PhotoBox*, bool rightMouseButton, bool doubleClick, void* );

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

	void setPhotoBoxClickedCallback( ClickCallback cb, void* data ) {
		_click_cb = cb;
		_click_cb_data = data;
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
			bool shift_pressed = Fl::event_key(FL_Shift_L);
			bool right_mouse = ( Fl::event_button3() != 0 );
			if( !shift_pressed && !right_mouse ) {
				unselectAll();
			}
			Fl_Widget* w = Fl::belowmouse();
			PhotoBox* box = dynamic_cast<PhotoBox*>( w );
			if( !box ) {
				Box* innerbox = dynamic_cast<Box*>( w );
				if( innerbox ) box = (PhotoBox*)innerbox->parent();
			}
			if( box ) {
				box->setSelected( true );
				if( _click_cb ) {
					(_click_cb)( box,
							     right_mouse,
							     (Fl::event_clicks() != 0),
								 _click_cb_data );
				}
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

	void getSelectedPhotos( vector<PhotoBox*>& selectedBoxes ) const {
		for( int i = 0, imax = children(); i < imax; i++ ) {
			PhotoBox* box = dynamic_cast<PhotoBox*>( child( i ) );
			if( box && box->isSelected() ) {
				selectedBoxes.push_back( box );
			}
		}
	}

private:
	ResizeCallback _resize_cb = NULL;
	void* _resize_cb_data = NULL;
	ScrollCallback _scroll_cb = NULL;
	void* _scroll_cb_data = NULL;
	ClickCallback _click_cb = NULL;
	void* _click_cb_data = NULL;
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
	MOVE_FILES,
	MOVE_FILES_BACK,
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
//	void fixButtonsOnResize() {
//		if( _filler == NULL )  {
//			int X = x() + w()/2;
//			_filler = new Fl_Box( X, y(), 1, 1 );
//			_filler->box( FL_FLAT_BOX );
//			_filler->color( FL_RED );
//			add( _filler );
//			resizable( _filler );
//		} else {
//
//		}
//	}

	FlxStatusBox* addStatusBox() {
		if( _statusbox == NULL ) {
			Position left = getXYfromLeft();
			Position right = getXYfromRight();
			int x = left.x + 10;
			int w = right.x - x;
			_statusbox = new FlxStatusBox( x, y() + 2, w, h()-4 );
			add( _statusbox );
			resizable( _statusbox );
			g_statusbox = _statusbox;
		}
		return _statusbox;
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
	FlxStatusBox* _statusbox = NULL;
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
		_scroll->setPhotoBoxClickedCallback( onPhotoBoxClicked_static, this );
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
		if( !pThis->getCurrentFolder().empty() ) {
			pThis->showFolderDialog();
		} else {
			g_statusbox->setStatusText( "You need to open a folder "
					                    "before you may create subfolders.");
		}
	}

	static void onRenameFiles_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		if( !pThis->getCurrentFolder().empty() ) {
			pThis->renameFiles();
		} else {
			g_statusbox->setStatusText( "Nothing to rename. No folder opened.", 2 );
		}
	}

	static void onMoveFiles_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->moveFiles();
	}

	static void onMoveFilesBack_static( Fl_Widget*, void* data ) {
		Controller* pThis = (Controller*) data;

	}

	static void onChangePage_static( Fl_Widget* btn, void* data ) {
		g_statusbox->setStatusText( "Changing page...", 2 );
		Controller* pThis = (Controller*)data;
		((Fl_Double_Window*)pThis->_scroll->parent())->cursor( FL_CURSOR_WAIT );
		Fl::check(); //give FLTK a little time to change cursor
		if( !pThis->checkEarmarks() ) return;
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
		((Fl_Double_Window*)pThis->_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	static void onPhotoBoxClicked_static( PhotoBox* box,
			                              bool rightMouse,
										  bool doubleClick,
										  void* data )
	{
		Controller* pThis = (Controller*) data;
		pThis->onPhotoBoxClicked( box, rightMouse, doubleClick );
	}

	void onPhotoBoxClicked( PhotoBox* box, bool rightMouse, bool doubleClick ) {
		if( !rightMouse && doubleClick ) {
			//zoom Image in FlxDialog
			//prepare dialog
			FlxDialog dlg( 300, _scroll->y(), 800, 800,
					      box->getPhotoPathnFile().c_str() );
			FlxRect& rect = dlg.getClientArea();
			PhotoBox box2( rect.x, rect.y, rect.w, rect.h );
			Fl_Color color = box->getSelectedColor();
			if( color ) {
				box2.setSelectedColor( box->getSelectedColor() );
			} else {
				const char* moveto = box->getMoveToFolder();
				if( moveto ) {
					box2.setMoveToFolder( moveto );
				}
			}
			Fl_Image* img = box->image();
			Size sz = box2.getPhotoSize();
			img->scale( sz.w, sz.h, 1, 1 );
			box2.image( img );
			dlg.add( box2 );

			//show dialog
			if( dlg.show( false ) ) {
				box->setSelectedColor( box2.getSelectedColor() );
				box->setMoveToFolder( box2.getMoveToFolder() );
			}

			//unset image and rescale it for displaying it in the main window
			box2.image( NULL );
			img->scale( box->w(), box->h(), 1, 1 );
			box->redraw();
		} else if( rightMouse && !doubleClick ) {
			//show context menu
			showPhotoContextMenu( box );
		}
	}

	void openFolder() {
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
		const char* folder = _folderManager.chooseFolder();
		if( folder ) {
			string title = "Photo Organization";
			title.append( ": " );
			title.append( folder );
			((Fl_Double_Window*)_scroll->parent())->label( title.c_str() );

			g_statusbox->setStatusTextV( "sss", "Reading photos from ", folder,
					                    ". This may take a while..." );

			/*get photos from selected dictionary*/
			readPhotos( folder );

			_toolbar->setManageFoldersButtonEnabled( true );
			_toolbar->setRenameFilesButtonEnabled( true );

			_folder.clear();
			_folder.append( folder );
		}
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	const string& getCurrentFolder() const {
		return _folder;
	}

	void renameFiles() {
		int resp = fl_choice( "Rename all photos?\n"
				              "Resulting filenames will be of format\nYYYYMMDD_TTTTTT.jpg",
							  "No", "Yes", NULL );
		if( resp == 1 ) {
			g_statusbox->setStatusText( "Renaming photos will take a while..." );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
			_folderManager.renameFilesToDatetime( _folder.c_str() );
			readPhotos( _folder.c_str() );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
		}
	}

private:

	void showPhotoContextMenu( PhotoBox* box ) {
		int nEntries = 1;
		vector<PhotoBox*> selectedBoxes;
		_scroll->getSelectedPhotos( selectedBoxes );
		if( selectedBoxes.size() > 1 ) {
			nEntries = 2;
		}
		Fl_Menu_Item menu[nEntries + 1];
		const char* DELETE = "Delete from disc";
		const char* COMPARE = "Compare selected photos...";
		menu[0] = { DELETE };
		if( nEntries == 2 ) {
			menu[1] = { COMPARE };
		}
		menu[nEntries == 1 ? 1 : 2] = { 0 };
		const Fl_Menu_Item *m = menu->popup( Fl::event_x(), Fl::event_y(),
											 0, 0, 0 );
		if( m ) {
			if( !strcmp( m->text, DELETE ) ) {
				int rc = fl_choice( "Really delete the selected photo(s)?",
						            "  No  ", "Yes", NULL );
				if( rc == 0 ) {
					return;
				}

				for( auto box : selectedBoxes ) {
					_folderManager.deleteFile( box->getPhotoPathnFile().c_str() );
				}
				readPhotos( _folder.c_str() );
			}
		}
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

	void showFolderDialog() {
		FolderDialog* dlg = new FolderDialog( 100, 100, _folder.c_str() );
		dlg->setCreateFolderCallback( FolderManager::onCreateFolders,
				                      &_folderManager );
		vector<string> folders;
		_folderManager.getFolders( _folder.c_str(), folders );
		dlg->setFolders( folders );
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

	void readPhotos( const char* folder ) {
		reset();
		vector<ImageInfo*>& imagefiles = _folderManager.getImages( folder );
		for( auto img : imagefiles ) {
			addImageFile( img->folder.c_str(), img->filename.c_str(), img->datetime.c_str() );
		}

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

	inline void validateEndIndex() {
		if( _photoIndexEnd >= (int)_photos.size() ) {
			_photoIndexEnd = _photos.size() - 1;
		}
	}

	inline void validateStartIndex() {
		_photoIndexStart = _photoIndexStart < 0 ? 0 : _photoIndexStart;
	}

	void loadPhoto( PhotoBox* box ) {
		Fl_JPEG_Image* jpg = new Fl_JPEG_Image( box->getPhotoPathnFile().c_str() );
		Size sz = box->getPhotoSize();
		jpg->scale( sz.w, sz.h, 1, 1 );
//		jpg->scale( _box_w, _box_h, 1, 1 );
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

	bool checkEarmarks() {
		auto itr = _photos.begin() + _photoIndexStart;
		auto itr2 = _photos.begin() + _photoIndexEnd;
		for( ; itr <= itr2 && itr != _photos.end(); itr++ ) {
			PhotoInfo* pinfo = (PhotoInfo*)(*itr);
			if( pinfo->box->getSelectedColor() != 0 ||
				pinfo->box->getMoveToFolder() != NULL )
			{
				int rc = fl_choice( "There are earmarked photos on this page.\n"
						            "Do you want them to be moved to the specified folder(s) "
						            "before paging or discard the earmarks?",
									"     Cancel     ", "   Move photos   ", "Discard earmarks" );
				if( rc == 0 ) { //Cancel
					return false;
				} else {
					if( rc == 1 ) {
						moveFiles();
					}
					return true;
				}

			}
		}
		//no earmarks
		return true;
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

	/**
	 * Move currently shown pictures according user's choices.
	 */
	void moveFiles() {
		if( _folder.empty() ) {
			g_statusbox->setStatusText( "Nothing to move. No folder opened.", 2 );
			return;
		} else {
			g_statusbox->setStatusText( "Moving files...", 2 );
		}
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
		int nmoved = 0;
		bool redfolderchecked = false;
		bool yellowfolderchecked = false;
		bool greenfolderchecked = false;
		bool checkerror = false;
		const char* srcfolder = _folder.c_str();
		auto itr = _photos.begin() + _photoIndexStart;
		auto itrmax = itr + _photoIndexEnd;
		for( ; itr != _photos.end() &&  itr <= itrmax && !checkerror; itr++ ) {
			PhotoInfo* pinfo = (PhotoInfo*)(*itr);
			PhotoBox* box = pinfo->box;
			Fl_Color color = box->getSelectedColor();
			string destfolder = _folder + "/";
			switch( color ) {
			case FL_RED:
				if( !redfolderchecked ) {
					if( !_folderManager.existsFolder(
							(_folder + "/" + GARBAGE_FOLDER ).c_str() ) )
					{
						fl_alert( "Garbage folder doesn't exist.\n"
								  "Create it using Toolbutton 'List and create subfolders'" );
						checkerror = true;
						break;
					}
					redfolderchecked = true;
				}
				destfolder.append( GARBAGE_FOLDER );
				break;
			case FL_YELLOW:
				if( !yellowfolderchecked ) {
					if( !_folderManager.existsFolder(
							(_folder + "/" + DUNNO_FOLDER ).c_str() ) )
					{
						fl_alert( "Dunno folder doesn't exist.\n"
								  "Create it using Toolbutton 'List and create subfolders'" );
						checkerror = true;
						break;
					}
					yellowfolderchecked = true;
				}
				destfolder.append( DUNNO_FOLDER );
				break;
			case FL_GREEN:
				if( !greenfolderchecked ) {
					if( !_folderManager.existsFolder(
							(_folder + "/" + GOOD_FOLDER ).c_str() ) )
					{
						fl_alert( "Good folder doesn't exist.\n"
								  "Create it using Toolbutton 'List and create subfolders'" );
						checkerror = true;
						break;
					}
					greenfolderchecked = true;
				}
				destfolder.append( GOOD_FOLDER );
				break;
			default:
				const char* dest = box->getMoveToFolder();
				if( !dest ) {
					continue;
				} else {
					destfolder.append( dest );
				}
			}
			if( !checkerror ) {
				nmoved++;
				_folderManager.moveFile( destfolder.c_str(),
										 srcfolder, pinfo->filename.c_str() );
			}
		}

		if( !checkerror && nmoved > 0 ) readPhotos( _folder.c_str() );

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}
private:
	Scroll* _scroll;
	ToolBar* _toolbar;
	int _box_w = 420;
	int _box_h = 420;
	int _spacing_x = 10;
	int _spacing_y = 5;

	int _photosPerRow = 3;
	int _maxRows = 4;

	std::vector<PhotoInfo*> _photos; //contains all photo infos of current folder.
	int _photoIndexStart = -1;
	int _photoIndexEnd = -1;
	long _usedBytes = 0;
	FolderManager& _folderManager = FolderManager::inst();
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

static void drawImage( Fl_Widget* w, void* data ) {
	static int x = 0;
	static int y = 0;
	static float zoom = 1.0;
	int n = 10;
	PhotoBox* box = (PhotoBox*)data;
	Fl_Image* img = box->image();
	FlxButton* btn = (FlxButton*) w;

	switch( btn->getId() ) {
	case 1: //xp
		x += n;
		break;
	case 2: //xm
		x -= n;
		break;
	case 3: //yp
		y += n;
		break;
	case 4: //ym
		y -= n;
		break;
	case 5: //zoom in
		zoom += 0.2;
		img->scale( box->getBox()->w() * zoom, box->getBox()->h() * zoom, 1, 1 );
		break;
	case 6: //zoom out
		zoom -= 0.2;
		img->scale( box->getBox()->w() * zoom, box->getBox()->h() * zoom, 1, 1 );
		break;
	default:
		break;
	}

	img->draw( x, y );
	box->getBox()->redraw();
	box->redraw();
//	box->parent()->redraw();
	fprintf( stderr, "drawImage: x = %d, y = %d\n", x, y );
}

int test() {
	Fl_Double_Window *win =	new Fl_Double_Window(200, 200, 400, 420, "TEST" );
	win->box( FL_FLAT_BOX );
	win->color( FL_LIGHT2 );
	int margin_x = 10;
	//int spacing_x = 5;
	int margin_y = 10;

	PhotoBox* box = new PhotoBox( margin_x, margin_y,
								  win->w() - 2*margin_x,
								  win->h() - 4*margin_y,
								  "/home/martin/Projects/cpp/Phorg/testphotos", "20200102_092128.jpg" );
	TestImage* img = new TestImage( "/home/martin/Projects/cpp/Phorg/testphotos/20200102_092128.jpg" );
	img->scale( box->w(), box->h(), 1, 1 );
	box->image( img );

	int y = box->y() + box->h() + 3;

	FlxButton* xp = new FlxButton( margin_x, y, 25, 25, "x+" );
	xp->setId( 1 );
	xp->callback( drawImage, box );
	FlxButton* xm = new FlxButton( xp->x() + xp->w(), y, 25, 25, "x-" );
	xm->setId( 2 );
	xm->callback( drawImage, box );

	FlxButton* yp = new FlxButton( xm->x() + xm->w() + 3, xm->y(), 25, 25, "y+" );
	yp->setId( 3 );
	yp->callback( drawImage, box );
	FlxButton* ym = new FlxButton( yp->x() + yp->w(), xm->y(), 25, 25, "y-" );
	ym->setId( 4 );
	ym->callback( drawImage, box );

	FlxButton* zi = new FlxButton( ym->x() + ym->w() + 10, ym->y(), 25, 25, "+" );
	zi->setId( 5 );
	zi->callback( drawImage, box );
	FlxButton* zo = new FlxButton( zi->x() + zi->w(), ym->y(), 25, 25, "-" );
	zo->setId( 6 );
	zo->callback( drawImage, box );

	win->end();
	win->show();
	return Fl::run();
}

/**
 * Application to get organization into your photos' folders.
 * View photo, delete it or move or copy it into another folder quickly.
 * Sort them in a conveniant way based on their names or date of taking.
 */
int main() {
	//return test();
	checkMemory();
	// additional linker options: -lfltk_images -ljpeg -lpng
	fl_register_images();
	int w = 1330;
	int h = 780;
	Fl_Double_Window *win =
			new Fl_Double_Window(150, 0, w, h, "Photo Organization" );

	ToolBar* tb = new ToolBar( 0, 0, w );
	Scroll* scroll = new Scroll( 0, tb->h(), w, win->h() - tb->h() );
	Controller ctrl( scroll, tb );
	tb->addButton( ToolId::OPEN_FOLDER, open_xpm, "Choose photo folder", Controller::onOpen_static, &ctrl );
	tb->addButton( ToolId::MANAGE_FOLDERS, manage_folders_xpm, "List, create and delete subfolders", Controller::onManageFolders_static, &ctrl );

	//Navigation buttons at the right edge of the toolbar
	tb->addButton( ToolId::LAST_PAGE, SYMBOL_LAST, FL_ALIGN_RIGHT, "Browse last page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::NEXT_PAGE, SYMBOL_NEXT, FL_ALIGN_RIGHT, "Browse next page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::PREVIOUS_PAGE, SYMBOL_PREVIOUS, FL_ALIGN_RIGHT, "Browse previous page", Controller::onChangePage_static, &ctrl );
	tb->addButton( ToolId::FIRST_PAGE, SYMBOL_FIRST, FL_ALIGN_RIGHT, "Browse first page", Controller::onChangePage_static, &ctrl );
	/////////////////////////////

	const char* f = "/home/martin/Projects/cpp/Phorg/images/rename_files.svg";
	tb->addButton( ToolId::RENAME_FILES, f, "Rename all jpg files in folder", Controller::onRenameFiles_static, &ctrl );
	const char* move = "/home/martin/Projects/cpp/Phorg/images/move.svg";
	tb->addButton( ToolId::MOVE_FILES, move, "Move earmarked pictures into specified folders", Controller::onMoveFiles_static, &ctrl );
	const char* move_back = "/home/martin/Projects/cpp/Phorg/images/move_back.svg";
	tb->addButton( ToolId::MOVE_FILES_BACK, move_back, "Move pictures from subfolders into parent folder", Controller::onMoveFilesBack_static, &ctrl );

	//tb->fixButtonsOnResize();
	FlxStatusBox* status = tb->addStatusBox();
	status->setStatusText( "Ready to rumble!" );

	tb->setAllPageButtonsEnabled( false );
	//tb->setManageFoldersButtonEnabled( false );
	//tb->setRenameFilesButtonEnabled( false );
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

/**
 * todo
 *  - write log on renamings
 *  - show number of photos to load and loading progress in status box
 *  - compare 2 photos in dialog (via context menu)
 *  - rotate photo (via context menu)
 *  - application icon
 */

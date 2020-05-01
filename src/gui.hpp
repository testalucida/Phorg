/*
 * gui.hpp
 *
 *  Created on: 01.05.2020
 *      Author: martin
 */

#ifndef GUI_HPP_
#define GUI_HPP_

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_Toggle_Button.H>
#include <fltk_ext/TextMeasure.h>
#include <fltk_ext/dragnresizehelper.h>
#include <fltk_ext/FlxStatusBox.h>
#include "std.h"
#include "global.h"
#include <cmath>

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

//+++++++++++++++++   BOX   ++++++++++++++++++++++++

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

	const string& getFolder() const {
		return _folder;
	}

	const string& getFile() const {
		return _file;
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



#endif /* GUI_HPP_ */

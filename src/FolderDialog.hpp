/*
 * FolderDialog.hpp
 *
 *  Created on: 22.04.2020
 *      Author: martin
 */

#ifndef FOLDERDIALOG_HPP_
#define FOLDERDIALOG_HPP_

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Button.H>
//#include <FL/Fl_Check_Button.H>
#include <fltk_ext/FlxCheckButton.h>

#include "std.h"

static const int W = 415;
static const int H = 300;

enum Folder {
	GARBAGE,
	GOOD,
	DUNNO
};

typedef void (*CreateFolderCallback) ( bool createGarbage, bool createGood, bool createDunno, const char* other, void* );

class FolderDialog: public Fl_Double_Window {
public:
	FolderDialog( int X, int Y, const char* currentFolder = NULL ) :
		Fl_Double_Window( X, Y, W, H, "Manage Folders" )
	{
		if( currentFolder ) {
			_currentFolder.append( currentFolder );
		}
		box( FL_FLAT_BOX );
		color(FL_LIGHT2);

		int margin_x = 10;
		int margin_y = 20;
		int spacing_x = 10;
		int spacing_y = 20;

		int x = margin_x;
		int y = margin_y;
		int h = 25;
		int w = W - 2*margin_x;

		_cbGarbage = createCheckButton( x, y, w, h,
				 "  Create garbage folder.\n"
				 "  Photos you'll delete will be put there.", Folder::GARBAGE );

		y += h + spacing_y;
		_cbGood = createCheckButton( x, y, w, h,
				 "  Create good folder.\n"
				 "  Photos you'll mark as 'good' will be put there.", Folder::GOOD );

		y += h + spacing_y;
		_cbDunno = createCheckButton( x, y, w, h,
				 "  Create dunno folder.\n"
				 "  Photos you'll mark as 'unsure' will be put there.", Folder::DUNNO );

		y += h + spacing_y;

		if( !_currentFolder.empty() ) {
			_note.append( ":\n" );
			_note.append( _currentFolder );
		} else {
			_note.append( "." );
		}
		new Fl_Box( x, y, W - 2*margin_x, 40, _note.c_str() );

		y += h + 2*spacing_y;
		Fl_Button* btn = createButton( x, y, 190, h, "Create selected folder(s)" );
		btn->callback( onCreateFolder_static, this );

		x += btn->w() + spacing_x;
		btn = createButton( x, y, 190, h, "Create other folder..." );
		btn->callback( onCreateOtherFolder_static, this );

		x = margin_x;
		y += h + 2*spacing_y;
		btn = createButton( x, y, 72, 25, "Close" );
		btn->callback( staticOkCancel_cb, this );

		end();
		size( this->w(), btn->y() + btn->h() + margin_y );
		//size( btn->x() + btn->w() + 2*margin_x, btn->y() + btn->h() + 2*margin_y );
	}

	~FolderDialog() {}

	void setCreateFolderCallback( CreateFolderCallback cb, void* data ) {
		_cfcb = cb;
		_cf_data = data;
	}

	void setFolderCheckBoxActive( Folder folder, bool activ ) {
		FlxCheckButton* btn = getCheckButton( folder );
		activ ? btn->activate() : btn->deactivate();
	}

	int show( bool modally ) {
		if( modally ) {
			Fl_Double_Window::set_modal();
			//Note: dialog will be placed int0 the middle of the
			//application disregarding given x and y position
		} else {
			Fl_Double_Window::set_non_modal();
		}

		Fl_Double_Window::show();
		//fprintf(stderr, "position x/y: %d/%d\n", x(), y());

		while( shown() ) {
			Fl::wait();
		}
		return 1; //_ok ? 1 : 0;
	}

private:
	static void onCreateFolder_static( Fl_Widget*, void* data ) {
		FolderDialog* pThis = (FolderDialog*)data;
		pThis->doCreateFolderCallback();
	}

	static void onCreateOtherFolder_static( Fl_Widget*, void* data ) {
		FolderDialog* pThis = (FolderDialog*)data;
		pThis->doCreateOtherFolderCallback();
	}

	void doCreateFolderCallback() {
		if( _cfcb ) {
			bool garb = ( _cbGarbage->active() && _cbGarbage->value() == 1 );
			bool good = ( _cbGood->active() &&  _cbGood->value() == 1 );
			bool dunno = ( _cbDunno->active() && _cbDunno->value() == 1 );
			(_cfcb)( garb, good, dunno, NULL, _cf_data );
			if( garb ) {
				_cbGarbage->deactivate();
			}
			if( good ) {
				_cbGood->deactivate();
			}
			if( dunno ) {
				_cbDunno->deactivate();
			}
		}
	}

	void doCreateOtherFolderCallback( const char* folder ) {
			if( _cfcb ) {
				(_cfcb)( false, false, false, folder, _cf_data );
			}
		}

	void doCreateOtherFolderCallback() {
		const char* folder = fl_input( "Enter folder's name: " );
		if( folder ) {
			doCreateOtherFolderCallback( folder );
		}
	}

	Fl_Button* createButton( int x, int y, int w, int h, const char* label ) {
		Fl_Button* btn = new Fl_Button( x, y, w, h, label );
		btn->box( FL_THIN_UP_FRAME );
		btn->down_box( FL_THIN_DOWN_FRAME );
		btn->color( (Fl_Color) 53 );
		btn->clear_visible_focus();

		return btn;
	}

	FlxCheckButton* createCheckButton( int x, int y, int w, int h, const char* label, Folder folder ) {
		FlxCheckButton* btn = new FlxCheckButton( x, y, w, h, label );
		btn->clear_visible_focus();
		btn->value( 1 );
		btn->setId( folder );
		return btn;
	}

	FlxCheckButton* getCheckButton( Folder folder ) const {
		if( _cbGarbage->getId() == folder ) {
			return _cbGarbage;
		}

		if( _cbGood->getId() == folder ) {
			return _cbGood;
		}

		if( _cbDunno->getId() == folder ) {
			return _cbDunno;
		}

		throw runtime_error( "FolderDialog::getCheckButton(): Folder " +
				              std::to_string( folder ) + " does not exist." );
	}

	static void staticOkCancel_cb(Fl_Widget* pWdgt, void* pUserData) {
		FolderDialog* pDlg = (FolderDialog*)pUserData;
		pDlg->okCancel_cb( (Fl_Button*)pWdgt );
	}

	void okCancel_cb( Fl_Button* pBtn ) {
	    //if( pBtn == _btnClose  ) {
			//_ok = true;
			hide();
//	    } else {
//	        _ok = false;
//	        hide();
//	    }
	}

private:
	string _currentFolder;
	FlxCheckButton* _cbGarbage = NULL;
	FlxCheckButton* _cbGood = NULL;
	FlxCheckButton* _cbDunno = NULL;
	string _note = "All folders will be created within the chosen photo folder";
	CreateFolderCallback _cfcb = NULL;
	void* _cf_data = NULL;
};


#endif /* FOLDERDIALOG_HPP_ */

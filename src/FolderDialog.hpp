/*
 * FolderDialog.hpp
 *
 *  Created on: 22.04.2020
 *      Author: martin
 */

#ifndef FOLDERDIALOG_HPP_
#define FOLDERDIALOG_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Browser.H>
//<1>
#include <FL/Fl_Native_File_Chooser.H>
//#include <FL/Fl_Check_Button.H>
#include <fltk_ext/FlxCheckButton.h>

#include "std.h"
#include "const.h"

static const int W = 415;
//static const int H = 300;
static const int H = 420;

enum Folder {
	GARBAGE,
	GOOD,
	DUNNO
};

typedef void (*CreateFolderCallback) ( bool createGarbage, bool createGood, bool createDunno, const char* other, void* );

class FolderDialog: public Fl_Double_Window {
public:
	FolderDialog( int X, int Y, const char* currentFolder ) :
		Fl_Double_Window( X, Y, W, H, "Manage Folders" )
	{

		_currentFolder.append( currentFolder );

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

		_browserLabel += "\n" + _currentFolder;
		new Fl_Box( x, y, W - 2*margin_x, 2*h, _browserLabel.c_str() );
		y += 2*h + 5;
		_browser = new Fl_Browser( x, y, W - 2*margin_x, 100 );
		_browser->end();

		y += _browser->h() + spacing_y;
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
		y = btn->y() + btn->h() + 2*spacing_y;
		_statusbox = new Fl_Box( x, y, W -2*margin_x, 25 );
		_statusbox->box( FL_THIN_DOWN_BOX );
		_statusbox->color( FL_LIGHT2 );
		_statusbox->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT );

		setStatus( "Ready." );

		y = _statusbox->y() + _statusbox->h() + 2*spacing_y;
		btn = createButton( x, y, 72, 25, "Close" );
		btn->callback( staticOkCancel_cb, this );

		end();
		size( this->w(), btn->y() + btn->h() + margin_y );
	}

	~FolderDialog() {}

	void setFolders( const vector<string>& folders ) {
		for( auto f : folders ) {
			_browser->add( f.c_str() );
		}
	}

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
		Fl::remove_timeout( resetStatus_static, this );
		return 1; //_ok ? 1 : 0;
	}

private:
	void setStatus( const char* msg ) {
		_statusbox->label( msg );
		Fl::add_timeout( 3.0, resetStatus_static, this );
	}

	static void resetStatus_static( void* data ) {
		FolderDialog* dlg = (FolderDialog*) data;
		dlg->resetStatus();
	}

	void resetStatus() {
		_statusbox->label( "" );
		_status.clear();
	}

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

			//todo <1>
			//check if folder is writable.
			//If not, show a new FileChooserDialog where user may
			//choose another (writable) folder.
			//The garb, good and dunno subfolders will be created in
			//the chosen write folder.

			(_cfcb)( garb, good, dunno, NULL, _cf_data );
			if( garb ) {
				_cbGarbage->deactivate();
				_browser->add( GARBAGE_FOLDER );
			}
			if( good ) {
				_cbGood->deactivate();
				_browser->add( GOOD_FOLDER );
			}
			if( dunno ) {
				_cbDunno->deactivate();
				_browser->add( DUNNO_FOLDER );
			}
			if( garb || good || dunno ) setStatus( "Folders created." );
			else setStatus( "No folders to create." );
		}
	}

	void doCreateOtherFolderCallback( const char* folder ) {
		if( _cfcb ) {
			//<1> FolderManager::onCreateFolder
			(_cfcb)( false, false, false, folder, _cf_data );
			_status = "Created folder ";
			_status.append( folder );
			_status.append( "." );
			_browser->add( folder );
			setStatus( _status.c_str() );
		}
	}

	void doCreateOtherFolderCallback() {
		//<1>
		//Replace fl_input by FileChooserDialog and let user choose
		//any folder and name.
		//const char* folder = fl_input( "Enter folder's name: " );
		const char* folder = NULL;
		Fl_Native_File_Chooser folderChooser( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
		folderChooser.options( Fl_Native_File_Chooser::NEW_FOLDER );
		switch ( folderChooser.show() ) {
		case -1: /*ERROR -- todo*/
			;
			break; // ERROR
		case 1: /*CANCEL*/
			fl_beep();
			break; // CANCEL
		default: // PICKED DIR
			//get selected folder name
			folder = folderChooser.filename();
		}
		// <1> end

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
	//todo <1>
	// change _currentFolder' meaning: it's the folder to read the photos from
	// but not necessarily the folder where good, dunno and garbage subfolders
	// are created.
	// Instead implement a new folder (string) with meaning "write" folder.

	string _browserLabel = "Existing folders in ";
	Fl_Browser* _browser = NULL;
	FlxCheckButton* _cbGarbage = NULL;
	FlxCheckButton* _cbGood = NULL;
	FlxCheckButton* _cbDunno = NULL;
	Fl_Box* _statusbox;
	string _note = "All folders will be created within the chosen photo folder";
	string _status;
	CreateFolderCallback _cfcb = NULL;
	void* _cf_data = NULL;
};


#endif /* FOLDERDIALOG_HPP_ */

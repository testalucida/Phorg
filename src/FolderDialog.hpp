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

static const int W = 400;
static const int H = 300;

class FolderDialog: public Fl_Double_Window {
public:
	FolderDialog( int X, int Y ) :
		Fl_Double_Window( X, Y, W, H, "Manage Folders" )
	{
		box( FL_FLAT_BOX );
		color(FL_LIGHT2);

		int spacing_x = 4;
		int x = spacing_x;
		int button_y = H - 30;
		_pBtnOk = new Fl_Return_Button( x, button_y, 72, 25, "OK" );
		_pBtnOk->box( FL_THIN_UP_FRAME );
		_pBtnOk->down_box( FL_THIN_DOWN_FRAME );
		_pBtnOk->color( (Fl_Color) 53 );
		_pBtnOk->callback( staticOkCancel_cb, this );

		x += _pBtnOk->w() + spacing_x;
		_pBtnCancel = new Fl_Button( x, button_y, 72, 25, "Cancel" );
		_pBtnCancel->box( FL_THIN_UP_FRAME );
		_pBtnCancel->down_box( FL_THIN_DOWN_FRAME );
		_pBtnCancel->color( (Fl_Color) 53 );
		_pBtnCancel->callback( staticOkCancel_cb, this );
		end();
	}

	~FolderDialog() {}

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
		return _ok ? 1 : 0;
	}

private:
	static void staticOkCancel_cb(Fl_Widget* pWdgt, void* pUserData) {
		FolderDialog* pDlg = (FolderDialog*)pUserData;
		pDlg->okCancel_cb( (Fl_Button*)pWdgt );
	}

	void okCancel_cb( Fl_Button* pBtn ) {
	    if( pBtn == _pBtnOk  ) {
			_ok = true;
			hide();
	    } else {
	        _ok = false;
	        hide();
	    }
	}

private:
	Fl_Return_Button *_pBtnOk=(Fl_Return_Button *)0;
	Fl_Button *_pBtnCancel=(Fl_Button *)0;
	bool _ok = false;
};


#endif /* FOLDERDIALOG_HPP_ */

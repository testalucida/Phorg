/*
 * Controller.hpp
 *
 *  Created on: 01.05.2020
 *      Author: martin
 */

#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_

#include "FolderManager.hpp"
#include "FolderDialog.hpp"
#include "gui.hpp"
#include "std.h"
#include "PhotoInfo.h"
#include "IFolderInfoProvider.h"
#include <fltk_ext/FlxDialog.h>

using namespace std;

enum Sort {
	SORT_ASC,
	SORT_DESC,
	SORT_NONE
};

static Sort _sortDirection = Sort::SORT_ASC;

class Controller : public IFolderInfoProvider {
private:

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
		//deletePhotoInfos();
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
			pThis->manageFolders();
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
		pThis->moveOrCopyFiles( true );
	}

	static void onMoveFilesBack_static( Fl_Widget* w, void* data ) {
		Controller* pThis = (Controller*) data;
		pThis->moveFilesBack();
	}

	static void onChangePage_static( Fl_Widget* btn, void* data ) {
		g_statusbox->setStatusText( "Changing page...", 2 );
		Controller* pThis = (Controller*)data;
		pThis->changePage( (Fl_Button*)btn);
	}

	static void onCreateFolders_static( bool garbage, bool good, bool dunno, const char* other, void* data ) {
		//<1> called by FolderDialog::doCreateOtherFolderCallback()
		//todo <1>
		// Logic is here to find either garbage (good, dunno) provided OR other.
		// Prospectively we have to handle the demand to create garbage, good and dunno within
		// other
		Controller* pThis = (Controller*)data;
		pThis->createFolders( garbage, good, dunno, other );
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
		vector<PhotoBox*> selectedBoxes;
		_scroll->getSelectedPhotos( selectedBoxes );

		if( !rightMouse && doubleClick ) {
			//zoom Image in FlxDialog
			showEnlargementDialog( selectedBoxes.at(0) );
		} else if( rightMouse && !doubleClick ) {
			//show context menu
			showPhotoContextMenu( selectedBoxes );
		}
	}

	void openFolder() {
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
		const char* folder = _folderManager.chooseFolder();
		if( folder ) {
			_folder.clear();
			_folder.append( folder );
			//<1>
			_writeFolder = _folder;

			string title = "Photo Organization";
			title.append( ": " );
			title.append( folder );
			((Fl_Double_Window*)_scroll->parent())->label( title.c_str() );

			g_statusbox->setStatusTextV( "sss", "Reading photos from ", folder,
					                    ". This may take a while..." );

			/*get photos from selected dictionary*/
			readPhotos();

			_toolbar->setManageFoldersButtonEnabled( true );
			_toolbar->setRenameFilesButtonEnabled( true );
		}
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	/**
	 * Returns the folder the photos were retrieved from
	 */
	const string& getCurrentFolder() const {
		return _folder;
	}

	//<1>
	void createFolders( bool garbage, bool good, bool dunno, const char* other ) {
		try {
			if ( garbage ) {
				_folderManager.createFolder( _writeFolder.c_str(), GARBAGE_FOLDER );
			}
			if( good ) {
				_folderManager.createFolder( _writeFolder.c_str(), GOOD_FOLDER );
			}
			if( dunno ) {
				_folderManager.createFolder( _writeFolder.c_str(), DUNNO_FOLDER );
			}
			if( other ) {
				_folderManager.createFolder( _writeFolder.c_str(), other );
			}
		} catch( FileExistsException& ex1 ) {
			fl_alert( "%s", ex1.what() );
		} catch( WritePermissionException& ex2 ) {
			fl_alert( "%s", ex2.what() );
		} catch( ReadOnlyException& ex3 ) {
			fl_alert( "%s", ex3.what() );
		} catch( FileWriteError& err ) {
			cerr << err.what() << endl;
			fl_alert( "%s", err.what() );
		}
	}

	/** implement from IFolderInfoProvider */
	virtual void getMoveToSubfolders( std::vector<string>& items ) {
		_folderManager.getFolders( _writeFolder.c_str(), items );
	}

	void renameFiles() {
		//todo <1> test
		if( !_folderManager.mayCurrentUserWrite( _folder.c_str() ) ) {
			string msg = _folder;
			msg.append( " is not writable.\nCan't rename files." );
			fl_alert( "%s", msg.c_str() );
			return;
		}

		int resp = fl_choice( "Rename all photos?\n"
				              "Resulting filenames will be of format\nYYYYMMDD_TTTTTT.jpg",
							  "No", "Yes", NULL );
		if( resp == 1 ) {
			g_statusbox->setStatusText( "Renaming photos will take a while..." );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
			_folderManager.renameFilesToDatetime( _photos );
			//readPhotos();
			layoutPhotos( Page::FIRST );
			((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
		}
	}

private:

	void changePage( Fl_Button* btn ) {
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
		Fl::check(); //give FLTK a little time to change cursor
		if( checkEarmarks() ) {
			int rc = fl_choice( "There are earmarked photos on this page.\n"
								"Do you want them to be moved to the specified folder(s) "
								"before paging or discard the earmarks?",
								"     Cancel     ", "   Move photos   ", "Discard earmarks" );
			if( rc == 1 ) {
				moveOrCopyFiles( false );
			}
		}
		string label;
		label.append( btn->label() );
		if( label == SYMBOL_NEXT ) {
			browseNextPage();
		} else if( label == SYMBOL_LAST ) {
			browseLastPage();
		} else if( label == SYMBOL_PREVIOUS ) {
			browsePreviousPage();
		} else {
			browseFirstPage();
		}
		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	void showPhotoContextMenu( vector<PhotoBox*>& selectedBoxes ) {
		Fl_Menu_Item menu[6];
		const char* GIMP = "Open with GIMP...";
		const char* DELETE = "Delete from disc";
		const char* COMPARE = "Compare selected photos...";
		const char* ROTATE_CLOCK = "Rotate clockwise";
		const char* ROTATE_ANTICLOCK = "Rotate anti-clockwise";
		menu[0] = { GIMP, 0, NULL, NULL, FL_MENU_DIVIDER };
		menu[1] = { DELETE, 0, NULL, NULL, FL_MENU_DIVIDER };
		menu[2] = { ROTATE_CLOCK };
		menu[3] = { ROTATE_ANTICLOCK, 0, NULL, NULL, FL_MENU_DIVIDER };
		menu[4] = { COMPARE };
		if( selectedBoxes.size() != 2 ) {
			menu[4].deactivate();
		}
		menu[5] = { 0 };
		const Fl_Menu_Item *m = menu->popup( Fl::event_x(), Fl::event_y(),
											 0, 0, 0 );
		if( m ) {
			if( !strcmp( m->text, GIMP ) ) {
				openWithGIMP( selectedBoxes );
			} else if( !strcmp( m->text, DELETE ) ) {
				int rc = fl_choice( "Really delete the selected photo(s)?",
						            "  No  ", "Yes", NULL );
				if( rc == 1 ) {
					for( auto box : selectedBoxes ) {
						_folderManager.deleteFile( box->getPhotoPathnFile().c_str() );
						removeFromPhotos( box->getFile() );
					}
					layoutPhotos( Page::FIRST );
				}
			} else if( !strcmp( m->text, ROTATE_CLOCK ) ) {
				rotate( selectedBoxes, 90 );
			} else if( !strcmp( m->text, ROTATE_ANTICLOCK ) ) {
				rotate( selectedBoxes, -90 );
			} else {
				//open enlargement dialog
				int n = selectedBoxes.size();
				showEnlargementDialog( selectedBoxes.at(0),
						               (n > 1) ?
						               selectedBoxes.at(1) : NULL );
			}
		}
	}

	void openWithGIMP( const vector<PhotoBox*>& boxes ) {
		string command = "gimp ";
		for( auto box : boxes ) {
			command.append( box->getPhotoPathnFile() );
			command.append( " " );
		}
		_gimp_handle = popen( command.c_str(), "r");
		if( !_gimp_handle ) {
			g_statusbox->setStatusText( "Failed opening GIMP." );
		}
	}

	void rotate( const vector<PhotoBox*>& boxes, int degrees ) {
		for( auto box : boxes ) {
			string command = "convert ";
			command.append( box->getPhotoPathnFile() );
			command.append( " -rotate " );
			command.append( to_string( degrees ) );
			command.append( " " );
			command.append( box->getPhotoPathnFile() );
			if( system( command.c_str() ) != 0 ) {
				g_statusbox->setStatusTextV( "ss", "Failed rotating ",
						                     box->getPhotoPathnFile().c_str() );
			} else {
				//reload rotated image
				Fl_Image* img = box->getBox()->image();
				delete img;
				box->getBox()->image( NULL );
				loadPhoto( box );
				box->redraw();
				g_statusbox->setStatusTextV( "ss", box->getFile().c_str(), " rotated." );
			}
		}
	}

	void showEnlargementDialog( PhotoBox* box1, PhotoBox* box2 = NULL ) {
		string title = box1->getFolder();
		title.append( ": " );
		title.append( box1->getFile() );
		if( box2 ) {
			title.append( ", " );
			title.append( box2->getFile() );
		}
		FlxDialog dlg( 300, _scroll->y(), 1400, 820,  title.c_str() );
		dlg.resizable( dlg );
		FlxRect& rect = dlg.getClientArea();

		//create new PhotoBox(es) to show in the enlargement dialog
		int n = box2 ? 2 : 1;
		PhotoBox box1n( rect.x, rect.y, rect.w/n, rect.h, *this );
		copyAttributes( &box1n, box1 );
		dlg.add( box1n );

		PhotoBox* box2n = NULL;
		if( box2 ) {
			int x = rect.x + box1n.w() + 3;
			int w = rect.w/2 - 3;
			box2n = new PhotoBox( x, rect.y, w, rect.h, *this );
			copyAttributes( box2n, box2 );
			dlg.add( box2n );
		}

		//2nd: show dialog
		if( dlg.show( false ) ) {
			box1->setSelectedColor( box1n.getSelectedColor() );
			box1->setMoveToFolder( box1n.getMoveToFolder() );
			if( box2 ) {
				box2->setSelectedColor( box2n->getSelectedColor() );
				box2->setMoveToFolder( box2n->getMoveToFolder() );
			}
		}

		//3rd: unset image and rescale it for displaying it in the main window
		box1n.image( NULL );
		box1->image()->scale( box1->w(), box1->h(), 1, 1 );
		box1->redraw();
		if( box2n ) {
			box2n->image( NULL );
			box2->image()->scale( box2->w(), box2->h(), 1, 1 );
			box2->redraw();
		}
	}

	void copyAttributes( PhotoBox* cpy, PhotoBox* orig ) {
		Fl_Color color = orig->getSelectedColor();
		if( color ) {
			cpy->setSelectedColor( orig->getSelectedColor() );
		} else {
			const char* moveto = orig->getMoveToFolder();
			if( moveto ) {
				cpy->setMoveToFolder( moveto );
			}
		}
		Fl_Image* img = orig->image();
		Size sz = cpy->getPhotoSize();
		img->scale( sz.w, sz.h, 1, 1 );
		cpy->image( img );
	}

	void layoutPhotos( Page page ) {
//		fprintf( stderr, "*********** clocking start in layoutPhotos ************\n" );
//		my::Timer timer;

		//remove and destroy all previous PhotoBoxes
//		timer.start();
		_copiedPhotos.clear();
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
		//fprintf( stderr, "******* layoutPhotos page %d *****************\n", (int)page );
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
										  *this,
						                  pinfo->folder.c_str(),
										  pinfo->filename.c_str(),
										  pinfo->datetime.c_str() );
				loadPhoto( pinfo->box );
				//fprintf( stderr, "Box instantiated: %s\n", pinfo->filename.c_str() );
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

		_currentPage = page;
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

	void manageFolders() {
		bool mayWrite = _folderManager.mayCurrentUserWrite( _writeFolder.c_str() );
		if( !mayWrite ) {
			//get another folder to create the subfolders in
			string msg = "Folder ";
			msg.append( _writeFolder ).append( " is not writable.\n" ).
			    append( "Choose or create another folder to move your photos to?" );

			int rc = fl_choice( "%s", "No thank you", //rc = 0
					            "Choose another folder", 0, msg.c_str() );
			if( rc == 0 ) return;

			_writeFolder = getWriteFolder();
			if( _writeFolder.empty() ) return;
		}

		FolderDialog* dlg = new FolderDialog( 100, 100, _writeFolder.c_str() );
		dlg->setCreateFolderCallback( onCreateFolders_static, this );
		vector<string> folders;
		_folderManager.getFolders( _writeFolder.c_str(), folders );
		dlg->setFolders( folders );
		string folder = _writeFolder;
		folder.append( "/" );
		string checkfolder = GARBAGE_FOLDER;
		if( _folderManager.existsFileOrFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::GARBAGE, false );
		}
		checkfolder = GOOD_FOLDER;
		if( _folderManager.existsFileOrFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::GOOD, false );
		}
		checkfolder = DUNNO_FOLDER;
		if( _folderManager.existsFileOrFolder( (folder + checkfolder).c_str() ) ) {
			dlg->setFolderCheckBoxActive( Folder::DUNNO, false );
		}

		dlg->show( true );
		delete dlg;
	}

	string getWriteFolder() {
		Fl_Native_File_Chooser folderChooser( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
		string folder;
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
		return folder;
	}

	/**
	 * 1st: Reads all photos from selected folder into vector _photos by calling
	 * FolderManager::getImages().
	 * FolderManager will provide folder and file infos including
	 * datetime info from EXIF block.
	 * 2nd: Rotates photos as neede by a call to FolderManager::rotate().
	 * 3rd: Sorts the photos in the _photos vector by datetime.
	 * Note: The PhotoBox object in each PhotoInfo object will be provided on demand
	 * for only the page to be displayed
	 */
	void readPhotos( /*const char* folder*/ ) {
		//reset();
		clearImages();
		_folderManager.getImages( _photos );
		if( _folderManager.mayCurrentUserWrite( _folder.c_str() ) ) {
			_folderManager.rotateImages();
		} else {
			g_statusbox->
			   setStatusTextV( "ss", _folder.c_str(),
					           " is not writeable.\n"
					           "Hence the images will not be rotated." );
		}
		sortImages( Sort::SORT_ASC );
		layoutPhotos( Page::FIRST );
	}

	void clearImages() {
		for( auto img : _photos ) {
			delete img;
		}
		_photos.clear();
	}

//	void reset() {
//		//clear the currently displayed photos
//		removePhotosFromCanvas();
//		//delete PhotoBox objects due to memory issues
//		//deletePhotoBoxes();
//		_usedBytes = 0;
//		_photoIndexStart = -1;
//		_photoIndexEnd = -1;
//	}

	static bool compare( const PhotoInfo* i1, const PhotoInfo* i2 ) {
		if( i1->datetime > i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? false : true;
		}
		if( i1->datetime < i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? true : false;
		}
		return ( i1->filename < i2->filename );
	}

	void sortImages( Sort sortDirection ) {
		_sortDirection = sortDirection;
		sort( _photos.begin(), _photos.end(), compare );
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
			if( ( pinfo->box->getSelectedColor() != 0 ||
				pinfo->box->getMoveToFolder() != NULL ) &&
				!alreadyCopied( pinfo ) )
			{
				return true;
			}
		}
		//no earmarks
		return false;
	}

	bool alreadyCopied( const PhotoInfo* pinfo ) const {
		for( auto copied : _copiedPhotos ) {
			if( copied == pinfo ) {
				return true;
			}
		}
		return false;
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

	void moveFilesBack() {
		if( _folder.empty() ) {
			g_statusbox->setStatusText( "Nothing to move back. No folder opened.", 2 );
			return;
		} else {
			bool mayMove = _folderManager.mayCurrentUserWrite( _folder.c_str() );
			if( !mayMove ) {
				g_statusbox->setStatusTextV( "sss", _folder.c_str(),
						                     " is not writeable.\n",
											 "Can't move pictures back to that folder." );
				return;
			} else {
				g_statusbox->setStatusText( "Moving files back...", 2 );
			}

		}

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );

		vector<string> folders;
		getFoldersToMoveBackFrom( folders );

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	void getFoldersToMoveBackFrom( vector<string>& folders ) {
		vector<string> subfolders;
		//get all subfolders
		_folderManager.getFolders( _writeFolder.c_str(), subfolders );
		int cnt = subfolders.size();
		int spacing_y = 10;
		int h = 25;
		int dlg_h = cnt * h + (cnt + 1 ) * spacing_y + 50;
		FlxDialog dlg( _scroll->top_window()->x() + Fl::event_x(),
				       _scroll->top_window()->y() + Fl::event_y() + 50,
					   400, dlg_h, "Choose Folders from where to move pics back" );
		Fl_Group* grp = dlg.createClientAreaGroup( FL_FLAT_BOX, FL_LIGHT2 );
		int x = grp->x() + 20;
		int y = grp->y() + spacing_y;
		FlxCheckButton* btn[cnt];
		for( int i = 0; i < cnt; i++ ) {
			btn[i] = new FlxCheckButton( x, y, 200, h, subfolders.at( i ).c_str() );
			btn[i]->clear_visible_focus();
			y += ( h + spacing_y );
		}

		int rc = dlg.show( false );
	}

	/**
	 * Move or copy currently shown pictures according user's choices.
	 * The pictures are moved if the folder containing them is writable, else copied.
	 */
	void moveOrCopyFiles( bool layoutAfterMoving ) {
		bool mayMove = true;
		if( _folder.empty() ) {
			g_statusbox->setStatusText( "Nothing to move/copy. No folder opened.", 2 );
			return;
		} else {
			//check if the containing folder is writeable.
			//if so, we may move the photos. Elsewise we have to copy.
			mayMove = _folderManager.mayCurrentUserWrite( _folder.c_str() );
			g_statusbox->setStatusText( mayMove ?
					                    "Moving files..." : "Copying files...",
										2 );
		}

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_WAIT );
		int nmoved = 0;
		bool redfolderchecked = false;
		bool yellowfolderchecked = false;
		bool greenfolderchecked = false;
		bool checkerror = false;
		const char* srcfolder = _folder.c_str();
		auto itr = _photos.begin() + _photoIndexStart;
		auto itrmax = itr + ( _photoIndexEnd - _photoIndexStart );
//		fprintf( stderr, "moveOrCopyFiles: photoIndexStart / photoIndexEnd: %d / %d\n",
//						       _photoIndexStart, _photoIndexEnd );

		vector<PhotoInfo*> to_remove;
		for( ; itr != _photos.end() &&  itr <= itrmax && !checkerror; itr++ ) {
			PhotoInfo* pinfo = (PhotoInfo*)(*itr);
			PhotoBox* box = pinfo->box;
			Fl_Color color = box->getSelectedColor();
			string destfolder = _writeFolder + "/";
			switch( color ) {
			case FL_RED:
				if( !redfolderchecked ) {
					if( !_folderManager.existsFileOrFolder(
							(_writeFolder + "/" + GARBAGE_FOLDER ).c_str() ) )
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
					if( !_folderManager.existsFileOrFolder(
							(_writeFolder + "/" + DUNNO_FOLDER ).c_str() ) )
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
					if( !_folderManager.existsFileOrFolder(
							(_writeFolder + "/" + GOOD_FOLDER ).c_str() ) )
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
				if( mayMove ) {
					_folderManager.moveFile( destfolder.c_str(),
											 srcfolder, pinfo->filename.c_str() );
					//earmark as to be removed
					to_remove.push_back( pinfo );
				} else {
					_folderManager.copyFile( destfolder.c_str(),
											srcfolder, pinfo->filename.c_str(),
											false );
					//As we copy only in case of a not writeable folder
					//we virtually remove this photo from its folder
					//by removing it from the photos vector
					to_remove.push_back( pinfo );
					_copiedPhotos.push_back( pinfo );
				}
			}
		} //for

		if( to_remove.size() > 0 ) {
			removeFromPhotos( to_remove );
		}

		if( layoutAfterMoving ) {
			layoutPhotos( _currentPage );
		}

		((Fl_Double_Window*)_scroll->parent())->cursor( FL_CURSOR_DEFAULT );
	}

	void removeFromPhotos( vector<PhotoInfo*>& to_remove  ) {
		for( auto itr = _photos.begin();
			 itr != _photos.end() && to_remove.size() > 0;
			 itr++ )
		{
			PhotoInfo* pi = *itr;
			for( auto itr2 = to_remove.begin(); itr2 != to_remove.end(); itr2++ ) {
				PhotoInfo* pi2 = *itr2;
				if( pi == pi2 ) {
					delete pi;
					itr = _photos.erase( itr ) - 1;
					itr2 = to_remove.erase( itr2 );
					_photoIndexEnd--;
					break;
				}
			} //inner for
		} //outer for
	}

	void removeFromPhotos( const string& filename ) {
		for( auto itr = _photos.begin(); itr != _photos.end(); itr++ ) {
			PhotoInfo* pinfo = *itr;
			if( pinfo->filename == filename ) {
				delete pinfo;
				_photos.erase( itr );
				break;
			}
		}
		throw runtime_error( "Controller::removeFromPhotos(filename):\n"
				             "PhotoInfo object not found." );
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
	//vector<ImageInfo*> _imageInfos; //contains all ImageInfo objects of current folder.
	std::vector<PhotoInfo*> _photos; //contains all photo infos of current folder.
	int _photoIndexStart = -1;
	int _photoIndexEnd = -1;
	Page _currentPage = Page::FIRST;
	long _usedBytes = 0;
	FolderManager& _folderManager = FolderManager::inst();
	string _folder; //current folder whose photos are displayed
	string _writeFolder; //may differ from _folder if _folder is not writable
	FILE* _gimp_handle = NULL;
	vector<PhotoInfo*> _copiedPhotos; //copied photos on the page displayed.
};


#endif /* CONTROLLER_HPP_ */

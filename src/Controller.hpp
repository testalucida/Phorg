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
#include <fltk_ext/FlxDialog.h>

using namespace std;

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

	void showPhotoContextMenu( vector<PhotoBox*>& selectedBoxes /*PhotoBox* box*/ ) {
		int nEntries = 2;
//		vector<PhotoBox*> selectedBoxes;
//		_scroll->getSelectedPhotos( selectedBoxes );
		if( selectedBoxes.size() > 1 ) {
			nEntries = 3;
		}
		Fl_Menu_Item menu[nEntries + 1];
		const char* GIMP = "Open with GIMP...";
		const char* DELETE = "Delete from disc";
		const char* COMPARE = "Compare selected photos...";
		menu[0] = { GIMP };
		menu[1] = { DELETE };
		if( nEntries == 3 ) {
			menu[2] = { COMPARE };
		}
		menu[nEntries == 2 ? 2 : 3] = { 0 };
		const Fl_Menu_Item *m = menu->popup( Fl::event_x(), Fl::event_y(),
											 0, 0, 0 );
		if( m ) {
			if( !strcmp( m->text, GIMP ) ) {
				openWithGIMP( selectedBoxes );
			} else if( !strcmp( m->text, DELETE ) ) {
				int rc = fl_choice( "Really delete the selected photo(s)?",
						            "  No  ", "Yes", NULL );
				if( rc == 0 ) {
					return;
				}

				for( auto box : selectedBoxes ) {
					_folderManager.deleteFile( box->getPhotoPathnFile().c_str() );
				}
				readPhotos( _folder.c_str() );
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
		PhotoBox box1n( rect.x, rect.y, rect.w/n, rect.h );
		copyAttributes( &box1n, box1 );
		dlg.add( box1n );

		PhotoBox* box2n = NULL;
		if( box2 ) {
			int x = rect.x + box1n.w() + 3;
			int w = rect.w/2 - 3;
			box2n = new PhotoBox( x, rect.y, w, rect.h );
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
	FILE* _gimp_handle = NULL;
};


#endif /* CONTROLLER_HPP_ */

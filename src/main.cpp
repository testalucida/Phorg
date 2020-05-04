
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_SVG_Image.H>
#include <fltk_ext/FlxButton.h>

#include "../images/open.xpm"
#include "../images/manage_folders.xpm"

#include "FolderManager.hpp"
#include "gui.hpp"
#include "Controller.hpp"

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
 *  - call FolderManager::getImages only once
 *    (from Controller::readPhotos())
 *  - Task 1 ( referenced as <1> )
 *    - handle write actions on non-writable folders (discs)
 *    - let user create folders beyond photos' folder
 *
 *  - move files back
 *  - write log on renamings
 *  - show number of photos to load and loading progress in status box
 *  - application icon
 */

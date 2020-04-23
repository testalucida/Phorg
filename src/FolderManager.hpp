/*
 * FolderManager.hpp
 *
 *  Created on: 21.04.2020
 *      Author: martin
 */

#ifndef FOLDERMANAGER_HPP_
#define FOLDERMANAGER_HPP_

#include "std.h"
#include "const.h"
#include <FL/Fl_Native_File_Chooser.H>
#include <stdexcept>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

enum Sort {
	SORT_ASC,
	SORT_DESC,
	SORT_NONE
};

static Sort _sortDirection = Sort::SORT_ASC;

struct ImageInfo {
	string folder;
	string filename;
	string datetime;
};

class FolderManager {
public:
	FolderManager() {
		_fileChooser.title( "Choose folder with photos to show" );
		_fileChooser.type( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
	}

	~FolderManager() {
		clearImages();
	}

	const char* chooseFolder() {
		// Show file chooser
		if( !_folder.empty() ) {
			_fileChooser.directory( _folder.c_str() );
		}
		switch ( _fileChooser.show() ) {
		case -1: /*ERROR -- todo*/
			;
			break; // ERROR
		case 1: /*CANCEL*/
			;
			fl_beep();
			break; // CANCEL
		default: // PICKED DIR
			_folder.clear();
			_folder.append( _fileChooser.filename() );
			return _folder.c_str();
		}

		return NULL;
	}

	/**
	 * Gets all jpg, jpeg JPG and JPEG files in the given folder.
	 * No png files.
	 * Renames files in a kind that the time taken forms the file name.
	 * Rotate jpeg if wanted and necessary and sort them if wanted
	 * ascending or descending according to  Date/Time info in the EXIF block.
	 * This is done by using jhead.
	 * Note:
	 * By each call of this method all PathnFile elements
	 * of the previously returned vector  will be deleted.
	 */
	vector<ImageInfo*>& getImages( const char* folder,
			                       bool rotate = true,
								   Sort sort = Sort::SORT_ASC )
    {
//		fprintf( stderr, "*********** clocking start in getImages ************\n" );
//		my::Timer timer;
		clearImages();

//		timer.start();
		if( rotate ) {
			rotateImages( folder );
		}
//		timer.stop();
//		fprintf( stderr, "time needed for rotateImages: %s\n",
//				         timer.durationToString());

//		timer.start();
		collectImageInfos( folder );
//		timer.stop();
//		fprintf( stderr, "time needed for collectImageInfos: %s\n",
//						         timer.durationToString());

//		timer.start();
		if( sort != Sort::SORT_NONE ) {
			sortImages( sort );
		}
//		timer.stop();
//		fprintf( stderr, "time needed for sortImages: %s\n",
//						         timer.durationToString());
//
//		fprintf( stderr, "*********** getImages: clocking end ************\n" );
		return _images;
	}

	/**
	 * Renames all photo files in the given folder so that their filename
	 * consists of the datetime token the photo.
	 * FolderManager::getImages() must have been called before
	 * otherwise no files will be renamed.
	 */
	void renameFilesToDatetime( const char* folder ) {
		for( auto ii : _images ) {
			if( ii->datetime != "unknown" ) {
				renameFile( ii->folder, ii->filename, ii->datetime );
			}
		}
	}

	/**
	 * Moves file filename from srcfolder to destfolder
	 */
	void moveFile( const char* destfolder, const char* srcfolder,
			       const char* filename )
	{
		string current = srcfolder;
		current.append( "/" );
		current.append( filename );
		string dest = destfolder;
		dest.append( "/" );
		dest.append( filename );
		if( rename( current.c_str(), dest.c_str() ) != 0 ) {
			perror( "Error moving file" );
			string msg = "Error on moving file from " + current + " to " + dest;
			throw runtime_error( "FolderManager::moveFile(): " + msg );
		}
	}

	static void onCreateFolders( bool garbage, bool good, bool dunno, const char* other, void* data ) {
		FolderManager* pThis = (FolderManager*) data;
		if ( garbage ) {
			pThis->createFolder( GARBAGE_FOLDER );
		}
		if( good ) {
			pThis->createFolder( GOOD_FOLDER );
		}
		if( dunno ) {
			pThis->createFolder( DUNNO_FOLDER );
		}
		if( other ) {
			pThis->createFolder( other );
		}
	}

	void getFolders( const char* parent, vector<string>& folders ) const {
		DIR *dir;
		struct dirent *ent;
		if ( (dir = opendir( parent )) != NULL ) {
			/* print all the files and directories within directory */
			while ( (ent = readdir( dir )) != NULL ) {
				string sub = ent->d_name;
				folders.push_back( sub );
			}
			closedir( dir );
		} else {
			string msg = "FolderManager::getFolders(): can't read ";
			msg.append( parent );
			/* could not open directory */
			perror( "Error reading directories" );
			throw runtime_error( msg );
		}
	}

	bool existsFolder( const char* folder ) const {
		struct stat buffer;
		return ( stat( folder, &buffer) == 0 );
	}

	void createFolder( const char* name ) const {
		int temp = umask( 0 );
		string folder = _folder;
		folder.append( "/" );
		folder.append( name );
		if ( mkdir( folder.c_str(), 0777 ) != 0 ) {
			string msg = "FolderManager::createFolder(): Error creating folder ";
			msg.append( name );
			perror( msg.c_str() );
			throw runtime_error( msg );
		}
		umask( temp );
	}

private:
	bool isImageFile( const char *filename ) {
		my::StringHelper &strh = my::StringHelper::instance();
		if ( strh.endsWith( filename, "jpg" )
				//|| strh.endsWith( filename, "png" )
				//|| strh.endsWith( filename, "PNG" )
				|| strh.endsWith( filename, "jpeg" )
				|| strh.endsWith( filename, "JPG" )
				|| strh.endsWith( filename, "JPEG " ) )
		{
			return true;
		}

		return false;
	}

	void clearImages() {
		for( auto img : _images ) {
			delete img;
		}
		_images.clear();
	}

	void rotateImages( const char* folder ) {
		string command = "jhead -autorot ";
		command.append( folder );
		command.append( "/*.*" );
		int rc = system( command.c_str() );
		if( rc != 0 ) {
			throw runtime_error( "FolderManager::rotate(): Rotation failed." );
		}
	}

	void collectImageInfos( const char* folder ) {
		string command = "jhead ";
		command.append( folder );
		command.append( "/*.*" );
		FILE* pipe;
		pipe = popen( command.c_str(), "r" );
		if( !pipe ) throw std::runtime_error( "FolderManager::collectImageInfos(): popen() failed!" );
		char buffer[128];
		string result = "";
		try {
			while (fgets( buffer, sizeof buffer, pipe ) != NULL) {
				result += buffer;
			}
		} catch (...) {
			pclose( pipe );
			throw;
		}
		pclose( pipe );

		provideImageInfos( result );
	}

	void getDateTimeInfo( const string& result, string& datetime, size_t start ) {
		const string datetime_const = "Date/Time";
		size_t pos1 = result.find( datetime_const, start );
		if( pos1 != string::npos ) {
			size_t posX = result.find( ":", pos1 ) + 2;
			if( posX != string::npos ) {
				size_t posEOL = result.find( "\n", posX );
				if( posEOL != string::npos ) {
					int len = posEOL - posX;
					datetime = result.substr( posX, len );
				} else {
					throw runtime_error( "FolderManager::getDateTimeInfo(): "
										 "Can't find end of line" );
				}
			} else {
				throw runtime_error( "FolderManager::getDateTimeInfo(): "
									 "Can't find colon" );
			}
		} else {
//			throw runtime_error( "FolderManager::getDateTimeInfo(): "
//								 "Can't find Date/Time string" );
			datetime = "unknown";
		}
	}

	void splitPathnfile( const string& pathnfile, string& path, string& file ) {
		size_t pos = pathnfile.rfind( "/" );
		path = pathnfile.substr( 0, pos );
		int len = pathnfile.size() - pos - 1;
		file = pathnfile.substr( pos+1, len );
	}

	void provideImageInfos( string& result ) {
		const string filename = "File name";

		size_t pos1 = result.find( filename, 0 );
		while( pos1 != string::npos ) {
			size_t posSlash = result.find( "/", pos1 + 1 );
			size_t posEOL;
			if( posSlash != string::npos ) {
				posEOL = result.find( "\n", posSlash + 1 );
				if( posEOL != string::npos ) {
					ImageInfo* ii = new ImageInfo;
					int len = posEOL - posSlash;
					string pathnfile( result, posSlash, len );
					splitPathnfile( pathnfile, ii->folder, ii->filename );
					getDateTimeInfo( result, ii->datetime, posEOL + 1 );
					_images.push_back( ii );
				} else {
					throw runtime_error( "FolderManager::provideImageInfos(): "
										 "Can't find end of line" );
				}
			} else {
				throw runtime_error( "FolderManager::provideImageInfos(): "
									 "Can't find start of path" );
			}
			pos1 = result.find( filename, posEOL + 1 );
		} // while
	}

	void renameFile( const string& folder, string& filename, const string& datetime ) {
		string old = folder;
		old.append( "/" );
		old.append( filename );
		filename.clear();
		getFilenameFromDatetime( filename, datetime );
		string ne = folder;
		ne.append( "/" );
		ne.append( filename );
		if( rename( old.c_str(), ne.c_str() ) != 0 ) {
			perror( "Error renaming file" );
			string msg = "Error on renaming file from " + old + " to " + ne;
			throw runtime_error( "FolderManager::renameFile(): " + msg );
		}
	}

	void getFilenameFromDatetime( string& newname, const string& dt ) {
		for( const char* p = dt.c_str(); *p; p++ ) {
			if( *p == ':' ) {
				continue;
			}
			if( *p == ' ' ) {
				newname.append( 1, '_' );
			} else {
				newname.append( 1, *p );
			}
		}
		newname.append( ".jpg" );
	}

	void sortImages( Sort sortDirection ) {
		_sortDirection = sortDirection;
		sort( _images.begin(), _images.end(), compare );
	}

	static bool compare( const ImageInfo* i1, const ImageInfo* i2 ) {
		if( i1->datetime > i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? false : true;
		}
		if( i1->datetime < i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? true : false;
		}
		return ( i1->filename < i2->filename );
	}

private:
	Fl_Native_File_Chooser _fileChooser;
	std::string _folder;
	std::vector<ImageInfo*> _images;
};



#endif /* FOLDERMANAGER_HPP_ */

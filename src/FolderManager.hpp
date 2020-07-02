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
#include "PhotoInfo.h"
#include <FL/Fl_Native_File_Chooser.H>
#include <stdexcept>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

class FileException : public exception {
public:
	FileException( const char* method, const char* name, const char* msg = NULL ) :
		exception(), _method(method), _name(name)
	{
		if( msg ) {
			_msg.append( msg );
		}
	}

	virtual ~FileException() { cerr << "FileException deleted." << endl; }

	virtual const char* what() const noexcept {
		_what = "FolderManager::";
		_what.append( _method ).append( ":\n" );
		_what.append( getType() ).append( " occured on writing " ).append( _name );
		if( !_msg.empty() ) {
			_what.append( "\n" ).append( _msg );
		}

		return _what.c_str();
	}

	virtual const char* getType() const = 0;
private:
	std::string _method;
	std::string _name;
	std::string _msg;
	mutable std::string _what;
};

class FileExistsException : public FileException {
public:
	FileExistsException( const char* method, const char* name, const char* msg = NULL ) :
		FileException( method, name, msg )
	{}

	~FileExistsException() { cerr << "FileExistsException deleted." << endl; }

	virtual const char* getType() const {
		return "FileExistsException";
	}
};

class WritePermissionException : public FileException {
public:
	WritePermissionException( const char* method, const char* name, const char* msg = NULL ) :
		FileException( method, name, msg )
	{}

	~WritePermissionException() { cerr << "WritePermissionException deleted." << endl; }

	virtual const char* getType() const {
		return "WritePermissionException";
	}
};

class ReadOnlyException : public FileException {
public:
	ReadOnlyException( const char* method, const char* name, const char* msg = NULL ) :
		FileException( method, name, msg )
	{}

	~ReadOnlyException() { cerr << "ReadOnlyException deleted." << endl; }

	virtual const char* getType() const {
		return "ReadOnlyException";
	}
};

class FileWriteError : public FileException {
public:
	FileWriteError( const char* method, const char* name, const char* msg = NULL ) :
			FileException( method, name, msg )
	{}

	~FileWriteError() { cerr << "FileWriteError deleted." << endl; }

	virtual const char* getType() const {
		return "FileWriteError";
	}
};

////////////////////////////////////////////////////////////
///////////////   FolderManager  ///////////////////////////
////////////////////////////////////////////////////////////
class FolderManager {
private:
	FolderManager() {
		_fileChooser.title( "Choose folder with photos to show" );
		_fileChooser.type( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
	}
public:
	~FolderManager() {
		//clearImages();
	}

	static FolderManager& inst() {
		static FolderManager* pThis = NULL;
		if( !pThis ) {
			pThis = new FolderManager();
		}
		return *pThis;
	}

	bool mayCurrentUserWrite( const char* folder ) {
//		char user[256];
//		int rc = getlogin_r( user, 256 );
//		if( rc == 0 ) {
			return( access( folder, W_OK ) == 0 );
//		}
//		return false;
	}

	/**
	 * Lets user choose a folder using Fl_Native_File_Chooser.
	 * (Ugly! Shouldn't be done by a management class.)
	 * A call to chooseFolder() sets the folder
	 * this FolderManager is working with.
	 */
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

	const string& getCurrentFolder() const {
		return _folder;
	}

	/**
	 * Gets all jpg, jpeg, JPG and JPEG files within the folder this
	 * FolderManager is working with.
	 * No png files.
	 */
	void getImages( vector<PhotoInfo*>& images ) {
		collectImageInfos( images );
	}

	void rotateImages( /*const char* folder*/ ) {
		string command = "jhead -autorot ";
		command.append( _folder.c_str() );
		command.append( "/*.*" );
		int rc = system( command.c_str() );
		if( rc != 0 ) {
			//we must not throw here because jhead gives an error 'no such file' if
			//folder is empty.
			//throw runtime_error( "FolderManager::rotateImages(): Rotation failed." );
		}
	}

	void copyFile( const char* destfolder, const char* srcfolder,
		           const char* filename, bool overwrite = true )
	{
		string src = srcfolder;
		src.append( "/" );
		src.append( filename );
		string dest = destfolder;
		dest.append( "/" );
		dest.append( filename );
		if( !overwrite ) {
			if( existsFileOrFolder( dest.c_str() ) ) return;
		}
		try {
			std::ifstream orig( src.c_str(), std::ios::binary );
			std::ofstream cpy( dest.c_str(), std::ios::binary );
			cpy << orig.rdbuf();
		} catch( ... ) {
			throw FileWriteError( "copyFile", dest.c_str(), NULL );
		}
	}

	/** Move .jpg files from srcfolder to destfolder */
	void moveJpegFiles( const char* destfolder, const char* srcfolder ) {
		vector<string> jpegs;
		getJpegFiles( srcfolder, jpegs );
		for( auto jpeg : jpegs ) {
			moveFile( destfolder, srcfolder, jpeg.c_str() );
		}
	}

	/**
	 * Moves file filename from srcfolder to destfolder
	 */
	void moveFile( const char* destfolder, const char* srcfolder,
			       const char* filename )
	{
		//todo <1>
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

	/**
	 * Renames all photo files in the given folder so that their filename
	 * consists of the datetime token the photo.
	 */
	void renameFilesToDatetime( vector<PhotoInfo*>& images ) {
		//<2>
		for ( auto ii : images ) {
			//todo <1>
			if ( ii->datetime != "unknown" ) {
				renameFile( ii->folder, ii->filename, ii->datetime );
			}
		}
	}


	void getFolders( const char* parent, vector<string>& folders ) const {
		DIR *dir;
		struct dirent *ent;
		if ( (dir = opendir( parent )) != NULL ) {
			while ( (ent = readdir( dir )) != NULL ) {
				if( ent->d_type == DT_DIR && ent->d_name[0] != '.' ) {
					string sub = ent->d_name;
					folders.push_back( sub );
				}
			}
			sort( folders.begin(), folders.end() );
			closedir( dir );
		} else {
			string msg = "FolderManager::getFolders(): can't read ";
			msg.append( parent );
			/* could not open directory */
			perror( "Error reading directories" );
			throw runtime_error( msg );
		}
	}

	/** Checks if the passed file or folder exists.
	 * The complete (absolute) path must be given. */
	bool existsFileOrFolder( const char* folder ) const {
		struct stat buffer;
		return ( stat( folder, &buffer) == 0 );
	}

	/** Deletes the passed file from disc.*/
	void deleteFile( const char* pathnfile ) {
		if( remove ( pathnfile ) != 0 ) {
			perror( "FolderManager::deleteFile(): error deleting file." );
			string msg = "FolderManager::deleteFile(): error deleting file ";
			msg.append( pathnfile );
			throw runtime_error( msg );
		}
	}

	/**
	 * Creates a subfolder named folder in folder parent.
	 * folder must only be the name of the subfolder not a complete path.
	 */
	void createFolder( const char* parent, const char* folder ) {
		string newfolder = parent;
		newfolder.append( "/" ).append( folder );
		createFolder( newfolder.c_str() );
	}

	void createFolder( const char* pathnfile /*<1> name*/ ) const {
		//called by FolderManager::onCreateFolders()
		int temp = umask( 0 );
		cerr << "going to create folder " << pathnfile << endl;
		if ( mkdir( pathnfile, 0777 ) != 0 ) {
			//<1>
			checkErrnoAndThrow( "createFolder", pathnfile );
		}
		umask( temp );
	}

private:
	//<1>
	void checkErrnoAndThrow( const char* method,
						     const char* name,
							 const char* msg = NULL ) const
	{
		switch( errno ) {
		case EACCES:
			//write permission is denied on the parent directory of the directory to be created.
			throw WritePermissionException( method, name, msg );
		case EEXIST:
			//named folder exists
			throw FileExistsException( method, name, msg );
		case EROFS:
			//The parent directory resides on a read-only file system
			throw ReadOnlyException( method, name, msg );
		default:
			string err = "errno = ";
			err.append( to_string( errno ) );
			throw FileWriteError( method, name, err.c_str() );
		}
	}
	//<1> end


	void getJpegFiles( const char* folder, vector<string>& files ) {
		DIR *dir;
		struct dirent *ent;
		if ( (dir = opendir( folder )) != NULL ) {
			while ( (ent = readdir( dir )) != NULL ) {
				if( ent->d_type == DT_REG ) {
					if( isJpeg( ent->d_name ) ) {
						files.push_back( ent->d_name );
					}
				}
			}

			closedir( dir );
		} else {
			/* could not open directory */
			perror( "Error reading directories" );
		}
	}

	bool isJpeg( const char* file ) {
		int len = strlen( file );
		const char* pmax = file + len;

		const char* p = file;
		for( ; p < pmax && *p != '.'; p++ );
		//p now pointing to '.'
		p++;

		if( p >= pmax || !( *p == 'j' || *p == 'J' ) ) return false;
		p++;
		if( p >= pmax || !( *p == 'p' || *p == 'P' ) ) return false;
		p++;
		if( p < pmax ) {
			if ( *p == 'e' || *p == 'E' ) {
				p++;
				if( p >= pmax ) return false;
			}
			if( *p == 'g' || *p == 'G' ) {
				if( *(p+1) == 0x00 ) return true;
			}
		}

		return false;
	}

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

	/**
	 * Creates a command calling jhead.
	 * jhead will read the jpg files in the folder this FolderManager
	 * is working with and extract the EXIF infos.
	 * We collect those infos by using a pipe.
	 */
	void collectImageInfos( vector<PhotoInfo*>& images ) {
		string command = "jhead ";
		command.append( _folder.c_str() );
		command.append( "/*.*" );
		FILE* pipe;
		pipe = popen( command.c_str(), "r" );
		if( !pipe ) {
			throw runtime_error( "FolderManager::collectImageInfos(): "
					             "popen() failed!" );
		}
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

		//get the interesting infos out of result
		provideImageInfos( result, images );
	}

	/**
	 * Extracts the result of a jhead call and creates ImageInfo objects
	 * for each EXIF block.
	 */
	void provideImageInfos( string& result, vector<PhotoInfo*>& images ) {
		string pathnfile;
		size_t eolAfterFile = -1;
		while( ( eolAfterFile =
				 getPathnFile( result, pathnfile, eolAfterFile+1 ) ) != string::npos )
		{
			PhotoInfo* ii = new PhotoInfo;
			splitPathnfile( pathnfile, ii->folder, ii->filename );
			getDateTimeInfo( result, ii->datetime, eolAfterFile + 1 );
//			fprintf( stderr, "ii filename and datetime: %s -- %s\n",
//					ii->filename.c_str(), ii->datetime.c_str() );
			images.push_back( ii );
		}
	}

	/**
	 * Searches for folder and file name within the given result
	 * starting from argument start.
	 * Provides argument pathnfile with the found path and file (if any)
	 * and returns the position of line end after file name.
	 */
	size_t getPathnFile( const string& result, string& pathnfile, size_t start ) {
		size_t posEOL = string::npos;
		size_t pos1 = result.find( "File name", start );
		if( pos1 != string::npos ) {
			size_t posSlash = result.find( "/", pos1 + 1 );
			if( posSlash != string::npos ) {
				posEOL = result.find( "\n", posSlash + 1 );
				if( posEOL != string::npos ) {
					int len = posEOL - posSlash;
					pathnfile = result.substr( posSlash, len );
				} else {
					throw runtime_error( "FolderManager::getFilename(): "
										 "Can't find end of line" );
				}
			} else {
				throw runtime_error( "FolderManager::getFilename(): "
									 "Can't find start of path" );
			}
		}
		return posEOL;
	}

	void splitPathnfile( const string& pathnfile, string& path, string& file ) {
		size_t pos = pathnfile.rfind( "/" );
		path = pathnfile.substr( 0, pos );
		int len = pathnfile.size() - pos - 1;
		file = pathnfile.substr( pos+1, len );
	}

	/** tries to extract date time infos out of the EXIF block.
	 * @param result: an unknown number of EXIF blocks to search in
	 * @param datetime: argument to provide with the date time info.
	 *                  If no date time info is found, "unknown" will be provided.
	 * @param start: position within exif where to start searching
	 */
	void getDateTimeInfo( const string& result, string& datetime, size_t start ) {
		const string datetime_const = "Date/Time";
		size_t pos1 = result.find( datetime_const, start );
		//check if we have found pos1 within the interesting EXIF block:
		string dummy;
		size_t check = getPathnFile( result, dummy, start );
		if( pos1 > check ) {
			//found Date/Time in one of the next EXIF blocks
			datetime = "unknown";
			return;
		}
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
			datetime = "unknown";
		}
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
			//todo <1>
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


private:
	Fl_Native_File_Chooser _fileChooser;
	std::string _folder;
//	std::vector<ImageInfo*> _images;
};



#endif /* FOLDERMANAGER_HPP_ */

#ifndef _CLASS_CZIPSTRUCTS
#define _CLASS_CZIPSTRUCTS

#define ID_PKZIPLFH 0x04034b50		// standard zip ids
#define ID_PKZIPCFH 0x02014b50
#define ID_PKZIPEND 0x06054b50

class CZipStructs
{
	public:

		#pragma pack(1)				// don't insert pad bytes !!!

		typedef struct tagZIP_LFH	// local fileheader entry (without ID and filename + extrafield)
		{
			WORD  version_needed_to_extract;
			WORD  general_purpose_bit_flag;
			WORD  compression_method;
			WORD  last_mod_file_time;
			WORD  last_mod_file_date;
			DWORD crc32;
			DWORD compressed_size;
			DWORD uncompressed_size;
			WORD  filename_length;
			WORD  extra_field_length;
		} ZIP_LFH;

		typedef struct tagZIP_CFH	// central directory entry (without ID and filename + extrafield + comment)
		{
			WORD  version_made_by;
			WORD  version_needed_to_extract;
			WORD  general_purpose_bit_flag;
			WORD  compression_method;
			WORD  last_mod_file_time;
			WORD  last_mod_file_date;
			DWORD crc32;
			DWORD compressed_size;
			DWORD uncompressed_size;
			WORD  filename_length;
			WORD  extra_field_length;
			WORD  file_comment_length;
			WORD  disk_number_start;
			WORD  internal_file_attributes;
			DWORD external_file_attributes;
			DWORD relative_offset_of_local_header;
		} ZIP_CFH;

		typedef struct tagZIP_EOF	// end of zipfile entry (with ID, without comment)
		{							// I should change the long names :O)
			DWORD end_of_central_dir_signature;
			WORD  number_of_this_disk;
			WORD  number_of_the_disk_with_the_start_of_the_central_directory;
			WORD  total_number_of_entries_in_the_central_dir_on_this_disk;
			WORD  total_number_of_entries_in_the_central_dir;
			DWORD size_of_the_central_directory;
			DWORD offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number;
			WORD  zipfile_comment_length;
		} ZIP_EOF;

		#pragma pack()
};

#endif

//
// Created by moss on 9/28/22.
//
#pragma once

#include <iostream>
#include <curl/curl.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include "color.h"
#include <sqlite3.h>
#ifdef DROPOUT_DL_GCRYPT
#include <gcrypt.h>
#endif

namespace dropout_dl {

	class cookie {
	public:
		/**
		 *
		 * @param data - A string to write to return value of the command to
		 * @param argc - The number of results from the command
		 * @param argv - The results of the command
		 * @param azColName - The column names from the command
		 * @return 0 on success or -1 on failure
		 *
		 * Used by sqlite. Write the first result of the sqlite command to the data string. If there are no results print an error and return -1.
		 */
		static int sqlite_write_callback(void* data, int argc, char** argv, char** azColName)
		{
			if (argc < 1) {
				std::cerr << "SQLITE ERROR: sqlite could not find desired cookie" << std::endl;
				return -1;
			}
			else {
				*(std::string*)data = argv[0];
				return 0;
			}
		}

		/**
		 * The name of the value from the sqlite database or "?" if not set.
		 */
		std::string name;
		/**
		 * The value of the cookie
		 */
		std::string value;
		/**
		 * The length of the value of the cookie
		 */
		int len;


		/**
		 *
		 * @param name - Name of the value from the sqlite database
		 *
		 * Create a cookie with no value and length of 0
		 */
		explicit cookie(const std::string& name) {
			this->name = name;
			this->value = "";
			this->len = 0;
		}

		/**
		 *
		 * @param name - Name of the value from the sqlite database
		 * @param cookie - Value of the cookie
		 *
		 * Sets the name and value using the parameters and gets the length from the value
		 */
		cookie(const std::string& name, const std::string& cookie) {
			this->name = name;
			this->value = cookie;
			this->len = cookie.size();
		}

		/**
		 *
		 * @param cookie - Value of the cookie
		 * @param length - Length of the cookie
		 *
		 * Sets the value and length using the parameters and sets the name as "?"
		 */
		cookie(const std::string& cookie, int length) {
			this->value = cookie;
			this->name = "?";
			this->len = length;
		}

		/**
		 *
		 * @param name - Name of the value from the sqlite database
		 * @param cookie - Value of the cookie
		 * @param length - Length of the cookie
		 *
		 * Sets the name, value, and length using the parameters leaving nothing unset.
		 */
		cookie(const std::string& name, const std::string& cookie, int length) {
			this->name = name;
			this->value = cookie;
			this->len = length;
		}

		/**
		 *
		 * @param db - An sqlite3 database
		 * @param sql_query_base - A base without the name search e.g. "FROM cookies" this function would then append the text "SELECT <value>" and "WHERE name='<name>'"
		 * @param value - The name of the value to fill the cookie with
		 *
		 * Retrieve the value of a cookie from the provided sqlite database.
		 *
		 */
		void get_value_from_db(sqlite3* db, const std::string& sql_query_base, const std::string& value, bool verbose = false, int (*callback)(void*,int,char**,char**) = sqlite_write_callback);

		/**
		 *
		 * @param password - Default is "peanuts". This works for linux. The password should be keychain password on MacOS
		 * @param salt - Salt is "saltysalt" for both MacOS and Linux
		 * @param length - Length of 16 is standard for both MacOS and Linux
		 * @param iterations - 1 on linux and 1003 on MacOS
		 *
		 * Decrypt chrome cookies and format them to be usable as regular cookies. Currently this has only been tested for the _session and __cf_bm cookies from dropout.tv but I see no reason this would not work for anything else.
		 */
		void chrome_decrypt(const std::string& password = "peanuts", int iterations = 1, const std::string& salt = "saltysalt", int length = 16);

	private:
		/**
		 * Remove url encoded text from a cookie. Currently this only checks for %3D ('=') as that is the only thing I've come across in cookies during the entirety of this project.
		 */
		void url_decode();

		/**
		 * Remove the leading version (e.g. "v10") from the cookie
		 */
		void format_from_chrome();
	};


	/**
	 *
	 * @param string - The string which is being searched
	 * @param start - The starting index of the substring
	 * @param test_str - The string which is being tested
	 * @return whether or not the substring is at the start index
	 *
	 * Checks if <b>test_str</b> is a substring of <b>string</b> at the start index
	 */
	bool substr_is(const std::string& string, int start, const std::string& test_str);

	/**
	 *
	 * @param str - The base string which is being modified
	 * @param from - what is being replaced
	 * @param to - what to place it with
	 *
	 * Replaces every instance of the <b>from</b> string with the <b>to</b> string.
	 */
	void replace_all(std::string& str, const std::string& from, const std::string& to);

	/**
	 *
	 * @param str - A string
	 * @return <b>str</b> with any leading or following whitespace
	 *
	 * Removes leading and following whitespace from a string and returns the modified string
	 */
	std::string remove_leading_and_following_whitespace(const std::string& str);

	/**
	 *
	 * @param str - A string
	 * @return <b>str</b> with any html character codes replaced with their ascii equivalent.
	 *
	 * E.G. \&#39; would be replaced with '
	 */
	std::string replace_html_character_codes(const std::string& str);

	/**
	 *
	 * @param str - A string
	 * @return <b>str</b> with junk removed or replace
	 *
	 * Removed leading and following whitespace and replaces html character codes
	 */
	std::string format_name_string(const std::string& str);

	/**
	 *
	 * @param str - A string
	 * @return <b>str</b> properly formatted to be a filename
	 *
	 * Removes non-alphanumeric characters and spaces
	 */
	std::string format_filename(const std::string& str);

	#if defined(__WIN32__)
	#include <windows.h>
	msec_t time_ms(void);
	#else
	#include <sys/time.h>
	/**
	 *
	 * @return The time in milliseconds
	 */
	long time_ms();
	#endif

	/**
	 *
	 * @param filename - Name of the file that is being downloaded
	 * @param total_to_download - The total amount of bytes that are being downloaded
	 * @param downloaded - The current amount of bytes that have been downloaded
	 * @param total_to_upload - The total amount of bytes that are being uploaded (This project does not upload so this is not used)
	 * @param uploaded - The current amount of bytes that have been uploaded (This project does not upload so this is not used)
	 * @return 0
	 *
	 * Used by curl. Displays the filename followed by a bar which show the percent downloaded followed by the number of Mib downloaded out of the total.
	 * The function takes the upload amount because that is required by curl but they are just ignored for this since we never upload anything.
	 */
	static int curl_progress_func(void* filename, double total_to_download, double downloaded, double total_to_upload, double uploaded);

	/**
	 *
	 * @param contents - What we're writing
	 * @param size - The amount that is being written
	 * @param nmemb - The number of bytes per unit of size
	 * @param userp - Where the information in <b>contents</b> is written to
	 * @return size * nmemb
	 *
	 * Used by curl. Writes the information gathered by curl into the userp string. This function was not written by me.
	 */
	size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

	/**
	 *
	 * @param url - Url which is being downloaded
	 * @param verbose - Whether or not to be verbose (not recommended)
	 * @return The page data as a string
	 *
	 * This function downloads the provided url and returns it as a string. Does not use cookies. This was ripped directly from a firefox network request for an episode page and modified minimally.
	 */
	std::string get_generic_page(const std::string& url, bool verbose = false);

	/**
	 * A class for handling all episode information. This class is wildly overkill if downloading an entire series as it gather the series name and season for every episode. This is not an issue here because all the information it gathers it already available while gathering the video url and the majority of the time taken while parsing an episode is from downloading the three required webpages.
	 */
	class episode {

	public:
		/// The name of the series that the episode belongs to
		std::string series;
		/// The directory for the series
		std::string series_directory;
		/// The name of the season that the episode belongs to
		std::string season;
		/// The number of the season (only set when downloading a season or series)
		int season_number = 0;
		/// The json metadata of the episode
		std::string metadata;
		/// The name of the episode
		std::string name;
		/// The number of the episode (only set when downloading a season or series)
		int episode_number = 0;
		/// The url for the main episode page
		std::string episode_url;
		/// The data of the main episode page
		std::string episode_data;
		/// The url for the main embedded page. This contains page the link to the config page
		std::string embedded_url;
		/// The data of the main embedded page. This contains the link to the config page
		std::string embedded_page_data;
		/// The url for the main config page. This contains page the link to the mp4 video of the episode
		std::string config_url;
		/// The data of the main config page. This contains the link to the mp4 video of the episode
		std::string config_data;
		/// The list of the qualities available for the episode. This is a parallel array with the quality_urls vector
		std::vector<std::string> qualities;
		/// The list of the urls correlating with the qualities array.
		std::vector<std::string> quality_urls;

		/// Whether or not to be verbose
		bool verbose = false;

		// Curl

		/**
		 *
		 * @param url - The url of the episode page
		 * @param auth_cookie - The authentication cookie with name "__cf_bm"
		 * @param session_cookie - The session cookie with name "_session"
		 * @param verbose - Whether or not to be verbose (not recommended)
		 * @return The episode page data
		 */
		static std::string get_episode_page(const std::string& url, const std::string& auth_cookie, const std::string& session_cookie, bool verbose = false);

		/**
		 *
		 * @param html_data - Episode page data
		 * @return The json data for the episode
		 *
		 * Gets the json metadata of the episode
		*/
		static std::string get_meta_data_json(const std::string& html_data);

		// Parsing
		/**
		 *
		 * @param meta_data - Episode metadata in json format
		 * @return The name of the series
		 *
		 * Get the name of the series from the metadata
		 */
		static std::string get_series_name(const std::string& meta_data);


		/**
		 *
		 * @param meta_data - Episode metadata in json format
		 * @return The name of the season
		 *
		 * Get the name of the season from the metadata
		 */
		static std::string get_season_name(const std::string& meta_data);

		/**
		 *
		 * @param meta_data - Episode metadata in json format
		 * @return The name of the episode
		 *
		 * Get the name of the episode from the metadata
		 */
		static std::string get_episode_name(const std::string& meta_data);

		/**
		 *
		 * @param html_data - Episode page data
		 * @return The url of the embedded page
		 *
		 * Get the url of the embedded page from the episode page
		 */
		static std::string get_embed_url(const std::string& html_data);

		/**
		 *
		 * @param html_data - Embedded page data
		 * @return The url of the config page
		 *
		 * Get the url of the config page from the embedded page data
		 */
		static std::string get_config_url(const std::string& html_data);

		/**
		 *
		 * @return A vector of qualities
		 *
		 * Gets the available qualities for the episode and populate the <b>qualities</b> and <b>quality_urls</b> vectors.
		 * If this function has already been run it simply returns the already populated <b>qualities</b> vector unless said vector has been cleared.
		 */
		std::vector<std::string> get_qualities();

		/**
		 *
		 * @param quality - The quality of the video
		 * @return The url to the video
		 *
		 * Get a link to the video of the episode with the given <b>quality</b>. <b>Quality</b> must be contained in the <b>qualities</b> vector otherwise this function will give an error and exit the program after listing the available qualities.
		 */
		std::string get_video_url(const std::string& quality);

		/**
		 *
		 * @param quality - The quality of the video
		 * @param filename - The filename which will be displayed will downloading the video
		 * @return The video data
		 *
		 * Download the episode with the given quality and return the raw video data as a string. The <b>filename</b> parameter is only used for displaying while downloading the video so that the user knows what is being downloaded. The <b>filename</b> argument is entirely optional and this function will not place the video into a file whether the value is given or not.
		 */
		std::string get_video_data(const std::string& quality, const std::string& filename = "");


		/**
		 *
		 * @param quality - The quality of the video
		 * @param base_directory - The directory which the episode is downloaded into
		 * @param filename - The name of the file (Will default if empty)
		 *
		 * Downloads the episode using the get_video_data function and places it into the <b>filename</b> file in the <b>base_directory</b> directory.
		 * If the file already exists it will output the name in yellow and will not redownload.
		 */
		void download_quality(const std::string& quality, const std::string& base_directory, const std::string& filename);

		/**
		 *
		 * @param quality - The quality of the video
		 * @param series_directory - The directory which the episode is downloaded into
		 * @param filename - The name of the file (Will default if empty)
		 *
		 * Downloads the episode using the get_video_data function and places it into the <b>filename</b> file in the <b>series_directory</b> directory.
		 * If the <b>filename</b> parameter is left empty it will default to the E\<episode_number\>\<name\>.mp4 format.
		 */
		void download(const std::string& quality, const std::string& series_directory, std::string filename = "");



		/**
		 *
		 * @param episode_url - Link to the episode
		 * @param cookies - The current cookies from the browser
		 * @param series - The series the episode is in
		 * @param season - The season the episode is in
		 * @param episode_number - The number of the episode
		 * @param season_number - The number of the season the episode is in
		 * @param verbose - Whether or not be verbose
		 *
		 * Create an episode object from the link using to cookies to get all the necessary information.
		 * This constructor initializes all the object data.
		 */
		episode(const std::string& episode_url, std::vector<cookie> cookies, const std::string& series, const std::string& season, int episode_number, int season_number, bool verbose = false) {
			this->episode_url = episode_url;
			this->verbose = verbose;

			episode_data = get_episode_page(episode_url, cookies[0].value, cookies[1].value);

			if (verbose) {
				std::cout << "Got page data\n";
			}

			metadata = get_meta_data_json(episode_data);

			if (verbose) {
				std::cout << "Got episode metadata: " << metadata << '\n';
			}

			name = get_episode_name(metadata);

			if (verbose) {
				std::cout << "Got name: " << name << '\n';
			}

			if (name == "ERROR") {
				std::cerr << "EPISODE ERROR: Invalid Episode URL\n";
				exit(6);
			}

			this->series = series;

			this->series_directory = format_filename(this->series);

			this->season = season;

			this->episode_number = episode_number;

			this->season_number = season_number;


			this->embedded_url = get_embed_url(episode_data);

			replace_all(this->embedded_url, "&amp;", "&");

			if (verbose) {
				std::cout << "Got embedded url: " << this->embedded_url << '\n';
			}

			this->embedded_page_data = get_generic_page(this->embedded_url);

			if (this->embedded_page_data.find("you are not authorized") != std::string::npos) {
				std::cerr << "ERROR: Could not access video. Try refreshing cookies.\n";
				exit(6);
			}

			this->config_url = get_config_url(this->embedded_page_data);

			replace_all(this->config_url, "\\u0026", "&");

			if (verbose) {
				std::cout << "Got config url: " << this->config_url << '\n';
			}

			this->config_data = get_generic_page(this->config_url);

			this->get_qualities();
			}

		/**
		 *
		 * @param episode_url - Link to the episode
		 * @param cookies - The current cookies from the browser
		 * @param verbose - Whether or not be verbose
		 *
		 * Create an episode object from the link using to cookies to get all the necessary information.
		 * This constructor initializes all the object data.
		 */
		episode(const std::string& episode_url, std::vector<cookie> cookies, bool verbose = false) {

			this->episode_url = episode_url;
			this->verbose = verbose;

			episode_data = get_episode_page(episode_url, cookies[0].value, cookies[1].value);

			if (verbose) {
				std::cout << "Got page data\n";
			}

			metadata = get_meta_data_json(episode_data);

			if (verbose) {
				std::cout << "Got episode metadata: " << metadata << '\n';
			}

			name = get_episode_name(metadata);

			if (verbose) {
				std::cout << "Got name: " << name << '\n';
			}

			if (name == "ERROR") {
				std::cerr << "EPISODE ERROR: Invalid Episode URL\n";
				exit(6);
			}

			this->series = get_series_name(metadata);

			if (verbose) {
				std::cout << "Got series: " << this->series << '\n';
			}

			this->season = get_season_name(metadata);

			if (verbose) {
				std::cout << "Got season: " << this->season << '\n';
			}

			this->series_directory = format_filename(this->series);

			if (verbose) {
				std::cout << "Got series directory: " << this->series_directory << '\n';
			}

			this->embedded_url = get_embed_url(episode_data);

			replace_all(this->embedded_url, "&amp;", "&");

			if (verbose) {
				std::cout << "Got embedded url: " << this->embedded_url << '\n';
			}

			this->embedded_page_data = get_generic_page(this->embedded_url);

			if (this->embedded_page_data.find("you are not authorized") != std::string::npos) {
				std::cerr << "ERROR: Could not access video. Try refreshing cookies.\n";
				exit(6);
			}

			this->config_url = get_config_url(this->embedded_page_data);

			replace_all(this->config_url, "\\u0026", "&");

			if (verbose) {
				std::cout << "Got config url: " << this->config_url << '\n';
			}

			this->config_data = get_generic_page(this->config_url);

			this->get_qualities();
		}

		/**
		 * Creates an episode object with no data. This should only be used for invalid states.
		 */
		episode() = default;
	};

} // dropout_dl

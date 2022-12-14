//
// Created by moss on 9/28/22.
//

#include "episode.h"


namespace dropout_dl {

	// dropout-dl helpers
	bool substr_is(const std::string& string, int start, const std::string& test_str) {
		if (test_str.size() != test_str.size())
			return false;

		for (int i = start, j = 0; i < start + test_str.size(); i++, j++) {
			if (string[i] != test_str[j]) {
				return false;
			}
		}
		return true;
	}

	void replace_all(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
	}


	std::string remove_leading_and_following_whitespace(const std::string& str) {
		int start = 0;
		int end = str.length() - 1;

		for (; str[start] == ' ' || str[start] == '\t' || str[start] == '\n'; start++);
		for (; str[end] == ' ' || str[end] == '\t' || str[end] == '\n'; end--);

		return str.substr(start, end - start + 1);
	}

	std::string replace_html_character_codes(const std::string& str) {
		std::string out;

		for (int i = 0; i < str.size(); i++) {
			if (substr_is(str, i, "&#")) {
				i += 2;
				char code = 0;

				if (i > str.size() - 4) {
					if (str[str.size() - 1] == ';') {
						// Numerical character code length is two at the end of the string

						code = str[str.size() - 2] - '0';
						code += (str[str.size() - 3] - '0') * 10;
						i += 2;
					}
				}
				else {
					if (str[i + 3] == ';') {
						// Numerical character code length is three
						code = str[i + 2] - '0';
						code += (str[i + 1] - '0') * 10;
						code += (str[i] - '0') * 10;
						i += 3;
					}
					else if (str[i + 2] == ';'){
						code = str[i + 1] - '0';
						code += (str[i] - '0') * 10;
						i += 2;
					}
					else {
						std::cerr << "HTML CHAR CODE ERROR: Code with numerical length of one used\n";
						exit(11);
					}
				}

				if (code < 32) {
					std::cerr << "HTML CHAR CODE ERROR: Control Character Decoded. This is not supported and likely an error.\n";
					exit(11);
				}

				out += code;
			}
			else {
				out += str[i];
			}
		}

		return out;
	}

	std::string remove_escaped_characters(const std::string& str) {
		std::string out;

		for (int i = 0; i < str.size(); i++) {
			if (str[i] == '\\') {
				i++;
			}
			out += str[i];
		}

		return out;
	}


	std::string format_name_string(const std::string& str) {
		return remove_escaped_characters(replace_html_character_codes(remove_leading_and_following_whitespace(str)));
	}

	std::string format_filename(const std::string& str) {
		std::string out;

		for (int i = 0; i < str.size(); i++) {
			char c = str[i];

			// Skip these
			if (c == '?' || c == ':' || c == '\\') {
				continue;
			}
			// Replace these with dashes
			else if (c == '/') {
				out += '-';
			}
			else {
				out += c;
			}
		}

		return out;
	}

#if defined(__WIN32__)
		#include <windows.h>
		msec_t time_ms(void)
		{
			return timeGetTime();
		}
	#else
	#include <sys/time.h>
		long time_ms()
		{
			timeval tv{};
			gettimeofday(&tv, nullptr);
			return tv.tv_sec * 1000 + tv.tv_usec / 1000;
		}
	#endif


	long current_time;
	long last_progress_timestamp;

	int curl_progress_func(void* filename, double total_to_download, double downloaded, double total_to_upload, double uploaded) {
		const double number_chars = 50;
		const char* full_character = "???";
		const char* empty_character = "???";

		current_time = time_ms();
		if (current_time - 50 > last_progress_timestamp) {
			// Percent of the file downloadede. Adding one to round up so that when its done it shows as a full bar rather than missing one.
			double percent_done = ((downloaded / total_to_download) * number_chars) + 1;
			double percent_done_clone = percent_done;
			std::cout << *(std::string*)filename << " [";
			while (percent_done_clone-- > 0) {
				std::cout << full_character;
			}
			while (percent_done++ < number_chars) {
				std::cout << empty_character;
			}
			std::cout << "] " << downloaded / 1048576 << "MiB / " << total_to_download / 1048576 << "MiB                           ";
			putchar('\r');
			last_progress_timestamp = time_ms();
			std::cout.flush();
		}
		return 0;
	}

	size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}

	std::string episode::get_meta_data_json(const std::string& html_data) {
		std::string data_start("window.Page = {");
		char data_open = '{';
		char data_close = '}';
		char current_char;
		// The current grouping depth. 1 because we only use it after we're inside the data brackets
		int grouping_depth = 1;
		for (int i = 0; i < html_data.size(); i++) {
			if (substr_is(html_data, i, data_start)) {
				i += data_start.size();
				for (int j = 0; j + i < html_data.size(); j++) {
					current_char = html_data[j + i];
					if (current_char == data_open) {
						grouping_depth++;
					}
					else if(current_char == data_close) {
						grouping_depth--;
					}

					if (grouping_depth == 0) {
						return(html_data.substr(i, j));
					}
				}
			}
		}
		return "ERROR";
	}

	// episode statics
	std::string episode::get_series_name(const std::string& meta_data) {
		int title_start = -1;
		std::string parent_title("\"parent\"");
		std::string series_title_title("\"name\"");
		for (int i = 0; i < meta_data.size(); i++) {
			if (substr_is(meta_data, i, parent_title)) {
				// Skip "VIDEO_TITLE", the following colon, and the opening quotation mark.
				i += parent_title.size() + 2;


				int j;
				for (j = 0; meta_data[i + j] != '}' && i + j < meta_data.size(); j++);

				std::string series_data = meta_data.substr(i, j);

				for (j = 0; j < series_data.size(); j++) {
					if (substr_is(series_data, j, series_title_title)) {
						// Skip "name", the following colon, and the opening quotation mark.
						j += series_title_title.size() + 2;

						int k;
						for (k = 0; j + k < series_data.size() && series_data[j + k] != '"'; k++);

						return format_name_string(series_data.substr(j, k));
					}
				}
			}
		}
		return "ERROR";
	}

	std::string episode::get_season_name(const std::string& meta_data) {
		std::string season_title_title("\"COLLECTION_TITLE\"");
		for (int i = 0; i < meta_data.size(); i++) {
			if (substr_is(meta_data, i, season_title_title)) {
				// Skip "VIDEO_TITLE", the following colon, and the opening quotation mark.
				i += season_title_title.size() + 2;


				int j;
				for (j = 0; meta_data[i + j] != '"' && i + j < meta_data.size(); j++);

				return format_name_string(meta_data.substr(i, j));
			}
		}
		return "ERROR";
	}

	std::string episode::get_episode_name(const std::string& meta_data) {
		std::string video_title_title("\"VIDEO_TITLE\"");
		for (int i = 0; i < meta_data.size(); i++) {
			if (substr_is(meta_data, i, video_title_title)) {
				// Skip "VIDEO_TITLE", the following colon, and the opening quotation mark.
				i += video_title_title.size() + 2;

				int j;
				for (j = 0; meta_data[i + j] != '"' && i + j < meta_data.size(); j++) {
					// skip checking for quotes if prefaced by a forward slash
					if (meta_data[i + j] == '\\') {
						j++;
					}
				}

				return format_name_string(meta_data.substr(i, j));
			}
		}
		return "ERROR";
	}

	std::string episode::get_embed_url(const std::string& html_data) {
		std::string config("window.VHX.config");
		std::string embed_url("embed_url: ");
		for (int i = 0; i < html_data.size(); i++) {
			if (substr_is(html_data, i, config)) {
				for (int j = i + config.size(); j < html_data.size(); j++) {
					if (substr_is(html_data, j, embed_url)) {
						for (int k = 0; k < html_data.size(); k++) {
							if (html_data[k + j + embed_url.size() + 1] == '"') {
								return html_data.substr(j + embed_url.size() + 1, k);
							}
						}
					}
				}
			}
		}
		return "";
	}

	std::string episode::get_config_url(const std::string& html_data) {
		std::string OTTdata("OTTData");
		std::string config_url("\"config_url\"");
		int remaining_quotes = 1;
		int url_start = -1;
		for (int i = 0; i < html_data.size(); i++) {
			if (substr_is(html_data, i, OTTdata)) {
				for (int j = i + OTTdata.size(); j < html_data.size(); j++) {
					if (substr_is(html_data, j, config_url)) {
						for (int k = 0; k < html_data.size() - (i + OTTdata.size()); k++) {
							char c = html_data[j + k + config_url.size()];
							if (remaining_quotes != 0) {
								if (html_data[j + k + config_url.size()] == '"') {
									remaining_quotes--;
								}
								continue;
							}
							else if (url_start == -1) {
								url_start = j + k + config_url.size();
							}

							if (html_data[url_start + k] == '"') {
								return html_data.substr(url_start, k);
							}
						}
					}
				}
			}
		}
		return "";
	}

	std::string episode::get_episode_page(const std::string& url, const std::string& auth_cookie, const std::string& session_cookie, bool verbose) {
		CURLcode ret;
		CURL *hnd;
		struct curl_slist *slist1;

		std::string episode_data;

		slist1 = nullptr;
		slist1 = curl_slist_append(slist1, "Accept: text/html");
		slist1 = curl_slist_append(slist1, "Accept-Language: en-US,en");
		slist1 = curl_slist_append(slist1, "Accept-Encoding: utf-8");
		slist1 = curl_slist_append(slist1, "DNT: 1");
		slist1 = curl_slist_append(slist1, "Connection: keep-alive");
		slist1 = curl_slist_append(slist1, ("Cookie: locale_det=en; _session=" + session_cookie + "; __cf_bm=" + auth_cookie).c_str());
		slist1 = curl_slist_append(slist1, "Upgrade-Insecure-Requests: 1");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Dest: document");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Mode: navigate");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Site: cross-site");
		slist1 = curl_slist_append(slist1, "Sec-GPC: 1");

		hnd = curl_easy_init();
		curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
		curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
		curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
		curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
		curl_easy_setopt(hnd, CURLOPT_FTP_SKIP_PASV_IP, 1L);
		curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(hnd, CURLOPT_VERBOSE, verbose);

		curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &episode_data);
		/* Here is a list of options the curl code used that cannot get generated
		   as source easily. You may choose to either not use them or implement
		   them yourself.

		CURLOPT_WRITEDATA set to a objectpointer
		CURLOPT_INTERLEAVEDATA set to a objectpointer
		CURLOPT_WRITEFUNCTION set to a functionpointer
		CURLOPT_READDATA set to a objectpointer
		CURLOPT_READFUNCTION set to a functionpointer
		CURLOPT_SEEKDATA set to a objectpointer
		CURLOPT_SEEKFUNCTION set to a functionpointer
		CURLOPT_ERRORBUFFER set to a objectpointer
		CURLOPT_STDERR set to a objectpointer
		CURLOPT_HEADERFUNCTION set to a functionpointer
		CURLOPT_HEADERDATA set to a objectpointer

		*/

		ret = curl_easy_perform(hnd);

		curl_easy_cleanup(hnd);
		hnd = nullptr;
		curl_slist_free_all(slist1);
		slist1 = nullptr;

		return episode_data;
	}

	std::string get_generic_page(const std::string& url, bool verbose) {
		CURL *hnd;
		struct curl_slist *slist1;

		std::string config_page;

		slist1 = nullptr;
		slist1 = curl_slist_append(slist1, "Accept: text/html");
		slist1 = curl_slist_append(slist1, "Accept-Language: en-US,en");
		slist1 = curl_slist_append(slist1, "Accept-Encoding: utf-8");
		slist1 = curl_slist_append(slist1, "DNT: 1");
		slist1 = curl_slist_append(slist1, "Connection: keep-alive");
		slist1 = curl_slist_append(slist1, "Referer: https://www.dropout.tv/");
		slist1 = curl_slist_append(slist1, "Upgrade-Insecure-Requests: 1");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Dest: iframe");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Mode: navigate");
		slist1 = curl_slist_append(slist1, "Sec-Fetch-Site: cross-site");
		slist1 = curl_slist_append(slist1, "Sec-GPC: 1");

		hnd = curl_easy_init();
		curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
		curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
		curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
		curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
		curl_easy_setopt(hnd, CURLOPT_FTP_SKIP_PASV_IP, 1L);
		curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(hnd, CURLOPT_VERBOSE, verbose);

		curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &config_page);

		curl_easy_perform(hnd);

		curl_easy_cleanup(hnd);
		hnd = nullptr;
		curl_slist_free_all(slist1);
		slist1 = nullptr;

		return config_page;
	}

	std::vector<std::string> episode::get_qualities() {
		if (!qualities.empty()) {
			return qualities;
		}
		int i = 0;
		bool video_section = false;

		const std::string quality_marker = R"("quality":")";
		for (; i < config_data.size(); i++ ) {
			// std::cout << i << "/" << javascript_data.size() << ": " << javascript_data[i] << ": " << javascript_data.substr(i, 17) << ": " << video_section << "\n";
			if (config_data.substr(i, 9) == "video/mp4") {
				video_section = true;
			}

			if (video_section && config_data.substr(i, quality_marker.size()) == quality_marker) {
				i += quality_marker.size();
				for (int j = 0; j + i < config_data.size(); j++) {
					if (config_data[i + j] == '"') {
						this->qualities.push_back(config_data.substr(i, j));
						if (this->verbose) {
							std::cout << "Found quality (" << i << " + " << j << "): " << qualities.back() << std::endl;
						}
						break;
					}
				}
				for (int j = i; j > 0; j--) {
					// std::cout << i << ": " << javascript_data[i] << ": " << javascript_data.substr(i-7, 7) << "\n";
					if (this->config_data.substr(j-7, 7) == R"("url":")") {
						for (int k = 0; k < i - j; k++) {
							if (config_data[j + k] == '"') {
								this->quality_urls.emplace_back(config_data.substr(j, k));
								if (this->verbose) {
									std::cout << "Found url (" << j << " + " << k << "): " << quality_urls.back()
											  << std::endl;
								}
								break;
							}
						}
						break;
					}
				}
			}
		}
		return qualities;
	}

	std::string episode::get_video_url(const std::string& quality) {
		for (int i = 0; i < qualities.size(); i++) {
			if (qualities[i] == quality) {
				return quality_urls[i];
			}
		}
		std::cerr << "ERROR: quality of " << quality << " not found\nPossible qualities: ";
		for (int i = 0; i < qualities.size(); i++) {
			std::cerr << qualities[i];
			if (i != qualities.size() - 1) {
				std::cerr << ", ";
			}
		}
		exit(6);
	}

	std::string episode::get_video_data(const std::string &quality, const std::string& filename) {
		CURL* curl = curl_easy_init();
		CURLcode res;
		if(curl) {
			std::string out;

			curl_easy_setopt(curl, CURLOPT_URL, get_video_url(quality).c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dropout_dl::WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, dropout_dl::curl_progress_func);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &filename);
			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);

			return out;
		}
		return "CURL ERROR";
	}


	void episode::download_quality(const std::string& quality, const std::string& base_directory, const std::string& filename) {
		if (!std::filesystem::is_directory(base_directory)) {
			std::filesystem::create_directories(base_directory);
			if (this->verbose) {
				std::cout << "Creating quality directory" << '\n';
			}
		}

		std::string filepath = base_directory + "/" + filename;

		if(std::filesystem::is_regular_file(filepath)) {
			std::cout << YELLOW << "File already exists: " << filepath << RESET << '\n';
			return;
		}
		std::fstream out(filepath,
						 std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

		out << this->get_video_data(quality, filepath) << std::endl;

		std::cout << GREEN << filepath << RESET;

		std::cout << '\n';
	}


	void episode::download(const std::string& quality, const std::string& series_directory, std::string filename) {
		if (filename.empty()) {
			if (this->episode_number != 0) {
				if (this->season_number != 0) {
					filename = this->series + " - S" + ((this->season_number < 10) ? "0" : "") + std::to_string(this->season_number) + "E" + ((this->episode_number < 10) ? "0" : "") + std::to_string(this->episode_number) + " - " + this->name + ".mp4";
				}
				else {
					filename = this->series + " - " + this->season + " Episode " + std::to_string(this->episode_number) + " - " + this->name + ".mp4";
				}
			}
			else {
				filename = this->series + " - " + this->season + " - " + this->name + ".mp4";
			}
			filename = format_filename(filename);
		}


		if (quality == "all") {
			for (const auto &possible_quality: this->qualities) {
				this->download_quality(possible_quality, series_directory + possible_quality, filename);
			}
		} else {
			this->download_quality(quality, series_directory, filename);
		}

	}



	// Cookie functions

	void cookie::get_value_from_db(sqlite3 *db, const std::string &sql_query_base, const std::string& value, bool verbose, int (*callback)(void*,int,char**,char**)) {
	#ifdef DROPOUT_DL_SQLITE
		std::string sql_mod_base = sql_query_base;

		if (sql_mod_base.find("WHERE") == std::string::npos) {
			sql_mod_base += " WHERE ";
		}
		else {
			sql_mod_base += " AND ";
		}

		sql_mod_base += "name='" + this->name + "';";

		std::string sql_value_query = "SELECT " + value + ' ' + sql_mod_base;
		std::string sql_length_query = "SELECT length(" + value + ") " + sql_mod_base;
		std::string tmp;
		char *err_code = nullptr;
		int rc;

		if (verbose) {
			std::cout << sql_value_query << '\n' << sql_length_query << std::endl;
		}

		rc = sqlite3_exec(db, sql_length_query.c_str(), callback, &tmp, &err_code);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", err_code);
			sqlite3_free(err_code);
			sqlite3_close(db);
			exit(3);
		} else if (verbose) {
			std::cout << "Got " << this->name << " cookie length\n";
		}

		if (tmp.empty()) {
			std::cerr << "COOKIE SQLITE ERROR: No Cookie With Name " << this->name << " Exists\n";
			exit(0);
		}
		this->len = std::stoi(tmp);

		rc = sqlite3_exec(db, sql_value_query.c_str(), callback, &tmp, &err_code);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", err_code);
			sqlite3_free(err_code);
			sqlite3_close(db);
			exit(3);
		} else if (verbose) {
			std::cout << "Got " << this->name << " cookie\n";
		}

		this->value = tmp;
	#else
		std::cerr << "COOKIE ERROR: Attempted to get cookies from sqlite without having sqlite installed\n";
		exit(12);
	#endif
	}

	void cookie::format_from_chrome() {
		this->value = this->value.substr(3);
		this->len -= 3;
	}

	void cookie::chrome_decrypt(const std::string &password, int iterations, const std::string &salt, int length) {

		#ifdef DROPOUT_DL_GCRYPT
		this->format_from_chrome();

		uint8_t key[32];

		char output[this->len + 2];

		char iv[16];

		for (char& c : iv) {
			c = ' ';
		}

		for (char& c : output) {
			c = 0;
		}

		gcry_kdf_derive(password.c_str(), password.size(), GCRY_KDF_PBKDF2, GCRY_KDF_ARGON2ID, salt.c_str(), salt.size(), iterations, length, key);

		gcry_cipher_hd_t handle;

		gcry_cipher_open(&handle, GCRY_CIPHER_AES, GCRY_CIPHER_MODE_CBC, 0);

		gcry_cipher_setkey(handle, (const void*) &key, length);

		gcry_cipher_setiv(handle, (const void*)&iv, 16);

		unsigned long err = gcry_cipher_decrypt(handle, output, this->len, this->value.c_str(), this->len);

		if (err) {
			std::cout << gcry_strerror(err) << std::endl;
			exit(2);
		}


		this->value = output;

		this->url_decode();

		this->value = this->value.substr(0, this->len - 7);
		this->len -= 7;
		#else
		std::cerr << "CHROME COOKIE ERROR: Attempted to Decrypt Chrome Cookie Without libgcrypt\n";
		exit(12);
		#endif
	}

	void cookie::url_decode() {
		std::string out;

		for (int i = 0; i < this->value.size() - 3; i++) {
			if (substr_is(this->value, i, "%3D")) {
				out += "=";
				i += 2;
			}
			else {
				out += this->value[i];
			}
		}

		this->value = out;
		this->len = out.size();
	}
} // dropout_dl

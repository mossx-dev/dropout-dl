# dropout-dl
dropout-dl is tool to download [dropout.tv](dropout.tv) episodes. It can download single episodes, seasons, or full series.


* [Installation](#installation)
  * [How to Build](#how-to-build)
  * [Dependencies](#Dependencies)



# Installation
## How to Build
```
cmake -S <source-dir> -B <build-dir>
cd <build-dir>
make
```

### Dependencies

### Required
* [cURL](https://curl.se/libcurl/) - Required for downloading pages and videos.
### Recommended
* [SQLite](https://www.sqlite.org/index.html) - Required for retrieving cookies from browsers. This can be avoided by using the `--force-cookies` [option](#options).
### Optional
* [libgcrypt](https://www.gnupg.org/software/libgcrypt/index.html) - Used for decrypting chrome cookies retrieved from the sqlite database.

#### Void
```
sudo xbps-install -S libcurl sqlite-devel libgcrypt
```

#### Debian
```
sudo apt install libcurl4-gnutls-dev sqlite-devel
```

## How to Use
```
./dropout-dl [options] <url>
```
By default, dropout-dl will download episodes in a season with the format `<series>/<season>/<series> - S<season-num>E<episode-num> - <episode-name>.mp4` and single episodes with the format `<series>/<season>/<series> - <season> - <episode-name>.mp4`.

### Options
```
--help                   Display this message
--quality                Set the quality of the downloaded video. Quality can be set to 'all' which
                             will download all qualities and place them into separate folders
--output                 Set the output filename
--output-directory       Set the directory where files are output
--verbose                Display debug information while running
--force-cookies          Interpret the next to arguments as authentication cookie and session cookie
--series                 Interpret the url as a link to a series and download all episodes from all seasons
--season                 Interpret the url as a link to a season and download all episodes in the season
--episode                Interpret the url as a link to a single episode
```

If series, season, or episode is not used, the type will be inferred based on the link format.

## Cookies
### Firefox
Create a file named `firefox_profile` in the build directory and paste in your [firefox profile folder path](https://support.mozilla.org/en-US/kb/profiles-where-firefox-stores-user-data)
### Chrome
Install libgcrypt and create a file named `chrome_profile` in the build directory and paste in your chrome profile folder path (found on [chrome://version](chrome://version))
### Other/No Sqlite
Use the `--force-cookies` program option to manually input cookies.

## TODO
- [x] Create tests
- [x] Handle non-alphanumeric characters
- [ ] Test build process on other setups with other OSs.




## Contributing
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](https://makeapullrequest.com)
### Issues
If you have any issues or would like a feature to be added please don't hesitate to submit an issue after checking to make sure it hasn't already been submitted. Using the templates is a good place to start, but sometimes they're overkill. For example, if the program segfaults for you, you don't need to state that the intended behaviour is to not segfault. \
\
If you'd like to contribute a good place to start is looking at open issues and trying to fix one with a pull request. \
**Working on your first Pull Request?** You can learn how from this *free* series [How to Contribute to an Open Source Project on GitHub](https://kcd.im/pull-request)

## Contributors
- Moss - [mossx-dev](https://github.com/mossx-dev)

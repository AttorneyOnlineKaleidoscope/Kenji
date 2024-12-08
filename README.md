# Kenji <img src="https://github.com/AttorneyOnlineKaleidoscope/Kenji/blob/master/resource/icon/256.png" width=30 height=30>
A C++ server for Attorney Online 2<br><br>
![Code Format and Build](https://github.com/AttorneyOnlineKaleidoscope/Kenji/actions/workflows/main.yml/badge.svg?event=push) [![Codecov branch](https://img.shields.io/codecov/c/gh/AttorneyOnlineKaleidoscope/Kenji/master)](https://app.codecov.io/gh/AttorneyOnlineKaleidoscope/Kenji) [![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/AttorneyOnlineKaleidoscope/Kenji/graphs/commit-activity) ![GitHub](https://img.shields.io/github/license/AttorneyOnlineKaleidoscope/Kenji?color=blue)<br>

# Where to download
You can find the latest stable release on our [release page.](https://github.com/AttorneyOnlineKaleidoscope/Kenji/releases)<br>
Nightly CI builds can be found at [Github Actions](https://github.com/AttorneyOnlineKaleidoscope/Kenji/actions)<br>

# Build Instructions
If you are unable to use either CI or release builds, you can compile Kenji yourself.<br>
Requires Qt >= 5.10, and Qt websockets

**Ubuntu 20.04/22.04** - Ubuntu 18.04 or older are not supported.
```
   sudo apt install build-essential qtbase5-dev qt5-qmake qttools5-dev qttools5-dev-tools libqt5websockets5-dev
   git clone https://github.com/AttorneyOnlineKaleidoscope/Kenji
   cd Kenji
   qmake project-Kenji.pro && make
```

# Contributors
![GitHub Contributors Image](https://contrib.rocks/image?repo=AttorneyOnlineKaleidoscope/Kenji)

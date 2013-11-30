purple-line
===========

libpurple (Pidgin, Finch) protocol plugin for LINE (http://line.naver.jp/en/) by Naver / NHN Japan. It's not quite featureful and stable enough to be used as a primary client for Line yet, but it's under heavy development.

The LINE protocol is closed, proprietary and there is no public API nor a truly cross-platform client (how lame!), so this repository shall also serve as unofficial documentation for the LINE protocol.

Does it work?
-------------

Yes, to an extent. Here's Pidgin logged in to LINE, but it's not quite stable and featureful enough to be used as one's main LINE client yet.

![Screenshot](http://virkkunen.net/b/pidgin-line2.png)

How to install
--------------

    make
    make install

This will build and install the plugin into your home directory. The prerequisites are a recent C++ compiler, the Apache Thrift compiler and libraries, and libpurple. Builds are only tested on Arch Linux for now.

Easy to install packages will not be available until I get all the main features implemented.

Implemented
-----------

* Logging in
  * Authentication
  * Fetching user profile
  * Account icon
  * Syncing friends, groups and chats
* Send and receive messages in IM, groups and chats
* Fetch recent messages
  * For groups and chats
* Synchronize buddy list on the fly
  * Adding friends
  * Blocking friends
  * Leaving groups
  * Leaving chats
* Buddy icons

To do
-----

* Fetch recent messages
  * For IMs
* Synchronize buddy list on the fly
  * Removing friends
  * Group invitations
  * Joining groups
  * Joining chats
* Editing buddy list
  * Adding friends (needs search function)
  * Removing friends
  * Creating groups
  * Updating groups
  * Inviting people to groups
  * Leaving groups
  * Creating chats
  * Inviting people to chats
  * Leaving chats
* Changing your account icon
* Other message types
  * Images: Thumbnail should be easy
  * Stickers: Figure out how to fetch, display as image. Send via commands?
  * Locations: Display as link
  * Audio/Video: Possibly difficult, low priority
* Emoji (is it possible to tap into the smiley system for sending too?)
* Companion Pidgin plugin
  * "Show more history" button
  * Enlarge images
  * Maybe even play audio and video messages
* Sending/receiving "message read" notifications
* Check builds on more platforms
* Packaging

**Warning: Unfinished software!** This plugin is still under development and many things are still unstable or unimplemented.

purple-line
===========

libpurple (Pidgin, Finch) protocol plugin for LINE (http://line.me/) by Naver / LINE Corporation.

Does it work?
-------------

Yes, to an extent. Here's Pidgin logged in to LINE, but it's not quite stable and featureful enough
to be used as one's main LINE client yet.

![Screenshot](http://virkkunen.net/b/pidgin-line2.png)

How to install
--------------

Make sure you have the required prerequisites:

* libpurple - probably available via package manager
* Apache Thrift compiler and C++ library
* line_main.thrift - not included, must be placed in the project root directory. For the time being,
  you can acquire this file from the documentation at http://altrepo.eu/

Then simply run:

    make
    make install

This will build and install the plugin into your home directory. Builds are only tested on Arch
Linux and a recent Ubuntu for now.

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
  * Removing friends
  * Joining chats
  * Leaving chats
  * Group invitations
  * Joining groups
  * Leaving groups
* Buddy icons
* Editing buddy list
 * Removing friends
 * Leaving chats
 * Leaving groups
* Message types
 * Text (send/receive)
 * Sticker (receive)

To do
-----

* Fetch recent messages
  * For IMs
* Synchronize buddy list on the fly
  * Sync group/chat users more gracefully, show people joining/leaving
* Editing buddy list
  * Adding friends (needs search function)
  * Creating chats
  * Inviting people to chats
  * Creating groups
  * Updating groups
  * Inviting people to groups
* Changing your account icon
* Message types
  * Image (send/receive)
  * Sticker (send) Send via commands/plugin?
  * Locations (send/receive) Display as link?
  * Audio/Video (send/receive) Receiving should be doable. File transfer API for sending?
  * Figure out what the other 15 message types mean...
* Emoji (is it possible to tap into the smiley system for sending too?)
* Companion Pidgin plugin
  * "Show more history" button
  * Enlarge images
  * Sticker list
  * Maybe even play audio and video messages
* Sending/receiving "message read" notifications
* Check builds on more platforms
* Packaging

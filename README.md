**Warning: Unfinished software!** This plugin is still under development and many things are still unstable or unimplemented.

purple-line
===========

libpurple (Pidgin, Finch) protocol plugin for LINE (http://line.me/) by Naver / LINE Corporation.

Where are the binaries and packages?
------------------------------------

I am not looking into "easy to install" options before I'm satisfied with the stability. I'd rather
not have people who cannot figure out how to compile software by themselves be disappointed by an
unstable plugin.

Does it work?
-------------

Yes, to an extent. Here's Pidgin logged in to LINE, but it's not quite stable and featureful enough
to be used as one's main LINE client yet.

![Screenshot](http://virkkunen.net/b/pidgin-line2.png)

How to install
--------------

Make sure you have the required prerequisites:

* libpurple - probably available via package manager
* Apache Thrift compiler and C++ library - v0.9.1 should be stable. The Git version and OS packages
  are sometimes a bit iffy. Compiling by hand is your best bet.
* line_main.thrift - not included, must be placed in the project root directory. For the time being,
  you can acquire this file from the documentation at http://altrepo.eu/

Then simply run:

    make
    make install

This will build and install the plugin into your home directory. Builds are only tested on Arch
Linux and a recent Ubuntu for now.

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
  * For IMs
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
 * Sticker (send via command/receive)
 * Image (receive preview)
 * Audio (receive preview)
 * Location (receive)

To do
-----

* Only fetch unseen messages, let a log plugin handle already seen messages
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
  * Image (send)
  * Audio/Video (send) File transfer API for sending?
  * Figure out what the other 15 message types mean...
* Emoji (is it possible to tap into the smiley system for sending too?)
* Companion Pidgin plugin
  * "Show more history" button
  * Sticker list
  * Open image messages
  * Open audio messages
  * Open video messages
* Sending/receiving "message read" notifications
* Check builds on more platforms
* Packaging

purple-line
===========

libpurple (Pidgin, Finch) protocol plugin for LINE (http://line.naver.jp/en/) by Naver / NHN Japan. It's not quite featureful and stable enough to be used as a primary client for Line yet, but it's under heavy development.

The LINE protocol is closed, proprietary and there is no public API nor a truly cross-platform client (how lame!), so this repository shall also serve as unofficial documentation for the LINE protocol.

Implemented
-----------

* Logging in
* Fetching user profile
* Initial friend list synchronization
* Sending and receiving text IMs
* Synchronizing groups
* Talking in groups
* Group member list

To do
-----

* Synchronizing things on the fly
* Adding/removing/blocking friends
* Inviting people to groups
* Joining/leaving groups
* Fetching recent messages when opening a conversation
* Sending/receiving "message read" notifications
* Other message types
  * Images: Thumbnail should be easy
  * Stickers: Figure out how to fetch, display as image
  * Locations: Display as link
* Companion Pidgin plugin:
  * "Show more history" button
  * Enlarge images
  * Maybe even play audio and video messages

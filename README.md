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
* Fetching recent messages when opening a group chat
* Synchronize buddy list on the fly
  * Add/block implemented as add/remove
* Buddy icons
* Account icon
* Handle exceptions from all thrift methods (especially AUTHENTICATION_DIVESTED_BY_OTHER_DEVICE)

To do
-----

* Changing your own icon
* Synchronize groups on the fly
* Add friends
  * Which search methods to support?
* Remove friend
* Create groups
* Invite people to groups
* Join groups (=accept invitations)
* Leave groups
* Other message types
  * Images: Thumbnail should be easy
  * Audio/Video: Possibly difficult, low priority
  * Stickers: Figure out how to fetch, display as image. Send via commands?
  * Locations: Display as link
* Emoji (is it possible to tap into the smiley system for sending too?)
* Companion Pidgin plugin:
  * "Show more history" button
  * Enlarge images
  * Maybe even play audio and video messages
* Sending/receiving "message read" notifications
* Refactor LineHttpTransport into a separate plain HTTP client and Thrift client
  * Also support non-SSL connections
* Packaging

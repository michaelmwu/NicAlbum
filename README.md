# Album Shuffle 1.3

Original by Nic Cleju. Modifications by Michael Wu

Simple complete albums & separate tracks shuffle for everyone.
 
Let's start from this basic assumptions:

1. You have many albums, each one in one folder
2. You use Dr.O's "Playlist Separator" plugin, which lets you separate them very nicely in your playlist [get it here, be sure to choose the latest release] and you want to:
3. Play one album track-by-track, then random pick another one, play it in order, and so on, without using a big complex plugin like "Albumlist". You just want to keep it simple, in your beautiful organized (with Dr.O's separators) playlist.
4. You also want to add to the playlist various single tracks, which should be randomly picked just like complete albums.

Well this is the plugin for you.

Just use Dr'O's "Playlist Separator" plugin [here] and make sure that:

1. Every album is preceeded by a separator, which starts with at least 5 minuses (for example "----- My Album -----", or simply "-----------")
2. A block of single tracks to be played randomly, not being part of an album, must pe preceeded by a separator which starts with at least five "equals" (for example "===== Tracks to be randomly selected ===="). You can create this with: Right Click -> Insert Separator, and put in the "New Entry" field something like "separator://?S====== ..."

The first entries in the playlist, before any separator, are considered random tracks.

Enable the plugin from the configuration window and just listen.

Things you should know:

1. The plugin works with JTFE.
2. The shuffling completely overrides Winamp's default shuffling. So nothing like "Shuffle Morph Rate" will work.
3. The "Previous" button allows 64 history entries. Old entries past 64 are forgotten. JTFE enqueued tracks are remembered in history.
4. Repeat Mode is ignored because there isn't really a way to tell Winamp there is no next track and to stop playing.
5. I use a Mersenne Twister algorithm to select the random playlist entry. See http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html.
6. To avoid playing the same file/album twice (by random), after one file/album is played it won't be played again until the whole playlist is played. One exception: the first file/album upon you double-click to start the playing is not taken into account.
7. Winamp crashes if your playlist is only separators. This is apparently is Winamp's fault and there is nothing to do about it. It goes into a stack overflow by trying to play the next separator until the stack gives out.
8. Winamp will crash if you attempt to sync a playlist with separators with an iPod or other portables. So remove the separators before syncing. This is not the plugin's fault.

Included is a Python Script to sort your playlists that have Album Shuffle blocks in them. It sorts by Path / Filename. The syntax is:

python sort.py PLAYLIST > OUTFILE
 
On Windows you need to output to a different file and copy it over the original, otherwise the original one will be erased

# streetrelay-3ds

Sources that helped tremendously while building this:

- [Gist by wwylele](https://gist.github.com/wwylele/29a8caa6f5e5a7d88a00bedae90472ed)
- [Docs by NarcolepticK](https://github.com/NarcolepticK/CECDocs/)
- [libctru-cecd by NarcolepticK](https://github.com/NarcolepticK/libctru-cecd)
- [CECTool by FlagBrew](https://github.com/FlagBrew/CECTool/)

If you're interested in building your own homebrew that works with StreetRelay,
I would recommend checking these sources out!

## Basics

### File Structure

```plain
CEC
+- MBoxList____
+- <typically 8 char id>
|  +- MBoxInfo____
|  +- MBoxData.001
|  +- MBoxData.010
|  +- MBoxData.050
|  +- InBox___
|  |  +- BoxInfo_____
|  |  +- _<12 char id>
|  |  +- ...
|  +- OutBox__
|     +- BoxInfo_____
|     +- OBIndex_____
|     +- _<12 char id>
+- <typically 8 char id>
   +- ...
```

1. `MBoxList____` contains a list of all the games that have StreetPass enabled
   currently, in the form of 16 byte IDs. These games are saved in folders,
   named the ASCII representation of their IDs.
2. `<typically 8 char ID>` is a folder containing all StreetPass-related
   information for a game, including:
   1. `MBoxInfo____` contains basic information about the mailbox
   2. `MBoxData.001` [ICN] encoded game icon for the title
   3. `MBoxData.010` UTF encoded game title
   4. `MBoxData.050` Title ID
   5. **`InBox___`** contains all current incoming StreetPass tags (i.e. a
      visitor in StreetPass Mii Plaza)
      1. `BoxInfo_____` contains information about the current box (inbox in
         this case), including an index of all messages and their headers.
      2. `_<12 char id>` is a StreetPass tag. (hurray) This file has a `0x70`
         length header, zero or more extra headers, and the actual tag data. The
         ID is the first 8 bytes of the message ID converted to a [custom
         base64][base64]
   6. **`OutBox__`** contains all? (never seen more than one) your outgoing
      StreetPass data. (i.e. your data as a visitor in StreetPass Mii Plaza)
      1. `BoxInfo_____` same as above, contains information about the outbox,
         and a list of all included messages' headers.
      2. `OBIndex_____` Only found in the `OutBox__`, contains a list of how
         many outbox items there are and a list of their IDs. Simplified version
         of `BoxInfo____`
      3. `_<12 char id>` outgoing StreetPass tag. Exactly the same as in the
         `InBox__`

[ICN]: https://www.3dbrew.org/wiki/SMDH#Icon_graphics
[base64]: https://github.com/NarcolepticK/CECDocs/blob/master/Misc/base64.md

## Workflow

1. Upload profile data (not relevant)
2. Get StreetPass games in `MBoxList___`
3. For each game in `MBoxList___`:
   1. Get outbox data
   2. Upload data in outbox
   3. Get list of tags available
   4. If server requests game public data:
      1. Upload `MBoxData.001` and `MBoxData.010` (icon and title)
   5. If inbox is not full:
      1. Download first tag
      2. Add tag to `BoxData____`
      3. Change tag dates

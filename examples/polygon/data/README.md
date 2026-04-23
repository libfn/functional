## Dictionary Files

Various dictionary files for use with the polygon puzzle solver.

| File | Source |
|---|---|
| `huge.txt` | [Jakub Mandula's diceware wordlist](https://github.com/mandulaj/diceware-v6/blob/31c49f9746ece2b23dae8c13066f81f8c5ee8e02/diceware-v6.txt.asc) |
| `large.txt` | [EFF wordlist](https://www.eff.org/deeplinks/2016/07/new-wordlists-random-passphrases) |
| `long.txt` | [Orchard Street wordlist](https://github.com/sts10/orchard-street-wordlists/blob/4fd015fe9a8e50d837d9f54cb39883bb801da1ed/lists/orchard-street-long.txt) |
| `short.txt` | [EFF wordlist](https://www.eff.org/deeplinks/2016/07/new-wordlists-random-passphrases) |
| `webster.txt` | [Webster's Unabridged Dictionary](https://github.com/matthewreagan/WebstersEnglishDictionary/blob/4fcf72dd08603a4c6885f691015b41c0b98ee4dc/WebstersEnglishDictionary.txt) |
| `words.txt` | [English lexicon](https://cs.uwaterloo.ca/~david/cs448/) |

`webster.txt` was additionally processed with the following shell command:

```sh
cat WebstersEnglishDictionary.txt \
  | sed 's/\r//g' \
  | grep -E '^[A-Z; -]{1,}$' \
  | sed 's/; /\n/g' \
  | sed 's/[A-Z]/\L&/g' \
  | sed 's/-//g' \
  | sort -u > webster.txt
```

Other files may also have been lightly processed to remove irrelevant data.

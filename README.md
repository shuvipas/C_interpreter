# C_interpreter

|          | Supported | Not Supported |
|----------|-----------|---------------|
|          |           | Macros        |
|          |           |function declaration (that means recursive calling between functions are not supported)
|          |           |               |
|numbers   |dec,hex,oct|               |
|          |           |               |
|escaped   | \n        | \t,\r etc     |
|characters|           |               |
|          |           |               |
|Comments  | //        | /* ...*/      |
|          |           |               |
|type      |  enum     |               |
|definitions|          |               |
|          |           |               |
|sizeof    |           |int, char,ptr  |
|          |           |               |
|          |           |               |
|          |           |               |
|          |           |               |

todo: add free to vm 
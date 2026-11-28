# Contract: iv command line

Unchanged by this feature. Both backends accept the same arguments, with the
same meaning, defaults and error messages. Argument parsing stays in
`ivmain.cpp` and is shared.

```
iv [options] [filename...]
```

| Option | Meaning |
|---|---|
| `filename...` | Image files or directories. A directory adds its readable images, sorted by name. |
| `-v` | Verbose status messages; prints memory and cache statistics at exit. |
| `-F` | Foreground mode (do not put the process in the background). |
| `--no-autopremult` | Turn off automatic premultiplication of unassociated alpha. |
| `--rawcolor` | Do not convert to RGB. |
| `--missing-pixels MODE` | `tolerate` (default) or `strict`. Any other value is an error and exits non-zero. |
| `--compare MODE` | Start out comparing the last two images: `wipe`, `absdiff` or `flipdiff`. Any other value is an error and exits non-zero. |
| `--display NAME` | OCIO display. |
| `--image-color-space NAME` | OCIO image color space. |
| `--view NAME` | OCIO view. |

Behavior that must hold in both backends:

- OCIO is enabled at startup only when all three of `--image-color-space`,
  `--display` and `--view` are given.
- If `$OCIO` is unset or names a missing file, `ocio://default` is used.
- A nonexistent file prints an error to stderr and is skipped.
- `--compare` takes effect shortly after the window is up (about 500 ms).
- Exit status is the application's exit status, 0 on normal close.

ImGui build only: when "Move to new window" starts a second iv process it
passes `-F`, the image path, and the current OCIO options.

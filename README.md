# explorer-file-ops

> Copy, move, and delete files and folders using the File Explorer progress GUI

This project is part of [#CreateWeekly](https://twitter.com/JosephusPaye/status/1214853295023411200), my attempt to create something new publicly every week in 2020.

## Why

It's easy to copy, move, and delete files and folders using the built-in [`fs`](https://nodejs.org/api/fs.html) module. But this module has a number of advantages over it:

- Shows the user progress in a familiar interface
- The user can pause and cancel the operation
- The user can undo and redo the operation
- The user will be prompted to authenticate or elevate to admin if access to the source or destination paths is restricted
- Deleted items go to the Recycle Bin by default
- Offloading the operation to File Explorer is more robust as it gracefully handles all the obscure errors that could occur in a user-friendly manner

## Installation

```
npm install @josephuspaye/explorer-file-ops --save
```

## Examples

## Copy files to a single destination

The following example also applies to the `move` function. The sources could also be folders, which would copy all their contents.

```js
const { copy } = require('@josephuspaye/explorer-file-ops');

// Copies `KillerBeanForever.mp4` and `BugsBunny.mp4` into a single destination
try {
  copy(
    [
      'C:\\Users\\jpaye\\Downloads\\KillerBeanForever.mp4',
      'C:\\Users\\jpaye\\Downloads\\BugsBunny.mp4',
    ],
    '\\\\MediaServer\\Movies'
  );
} catch (err) {
  console.error('unable to copy files: ' + err.message);
}
```

## Move multiple source files to multiple destinations

The following example also applies to the `copy` function. The sources could also be folders.

```js
const { move } = require('@josephuspaye/explorer-file-ops');

// Moves `C:\source-a\a.zip` to `X:\destination-a\new-a.zip` and
// `C:\source-b\b.iso` to `X:\destination-b\new-b.iso`
try {
  move(
    ['C:\\source-a\\a.zip', 'C:\\source-b\\b.iso'],
    ['X:\\destination-a\\new-a.zip', 'X:\\destination-b\\new-b.iso']
  );
} catch (err) {
  console.error('unable to move files: ' + err.message);
}
```

## Delete files, moving to the Recycle Bin

```js
const { del } = require('@josephuspaye/explorer-file-ops');

// Deletes `C:\source-a\a.zip` to `X:\disposable-directory\`
try {
  del(['C:\\source-a\\a.zip', 'X:\\disposable-directory\\']);
} catch (err) {
  console.error('unable to delete files: ' + err.message);
}
```

## Disable error dialog

By default, copy, move, and delete operations that result in an error will show the user a message box with the error message. You can disable this behaviour as follows:

```js
const { copy } = require('@josephuspaye/explorer-file-ops');

// Attempts to copy the file without showing an error when the target
// directory is not found. Note that this won't throw, as errors are
// only thrown when the input is invalid. Actual errors with the file
// operation are handled by Explorer or the custom error dialog
try {
  copy('C:\\source\\a.zip', 'X:\\non-existent-directory', {
    showDialogOnError: false,
  });
} catch (err) {
  console.error('unable to copy files: ' + err.message);
}
```

## API

```ts
interface FileOpOptions {
  /**
   * Show the user an error dialog if there's an error with the operation
   * @default true
   */
  showDialogOnError?: boolean;
}

/**
 * Copy the given source path(s) to the given destination path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
function copy(
  src: string | string[],
  dest: string | string[],
  options?: FileOpOptions
): Promise<number | null>;

/**
 * Move the given source path(s) to the given destination path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
function move(
  src: string | string[],
  dest: string | string[],
  options?: FileOpOptions
): Promise<number | null>;

/**
 * Delete the given source path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
function del(
  src: string | string[],
  options?: FileOpOptions
): Promise<number | null>;
```

## Building the executable

The module uses an executable to launch the properties dialog for the given path. The source of this executable is at [src/fileops.cpp](src/fileops.cpp) and you can build it as follows:

- Install an MSVC Compiler. You can get this with [windows-build-tools](https://www.npmjs.com/package/windows-build-tools) or Visual Studio.
- Copy the `.env.bat.example` file to `.env.bat` and update the variables to match your system
- Run `./build.bat` to build. The resulting executable will be placed at `bin/FileOps.exe`.

## Licence

[MIT](LICENCE)

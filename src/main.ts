import path from 'path';
import { commandsAsScript } from '@josephuspaye/powershell';

export interface FileOpOptions {
  /**
   * Show the user an error dialog if there's an error with the operation
   * @default true
   */
  showDialogOnError?: boolean;
}

const exe = path.join(__dirname, '..', 'bin', 'FileOps.exe');

/**
 * Check the given inputs and throw an error if they're not valid
 */
function validateInput(
  action: 'copy' | 'move' | 'delete',
  src: string | string[],
  dest: string | string[]
) {
  src = src ?? '';
  dest = dest ?? '';

  if (typeof src === 'string') {
    src = src.trim();
  }

  if (typeof dest === 'string') {
    dest = dest.trim();
  }

  const srcPaths = ([] as string[]).concat(src.length > 0 ? src : []);
  const destPaths = ([] as string[]).concat(dest.length > 0 ? dest : []);

  if (srcPaths.length == 0) {
    throw new Error('at least one source path is required');
  }

  if (action !== 'delete' && destPaths.length == 0) {
    throw new Error(
      'at least one destination path is required when copying or moving'
    );
  }

  if (destPaths.length > srcPaths.length) {
    throw new Error(
      'number of destination paths cannot be more than number of source paths'
    );
  }

  if (
    srcPaths.length > 1 &&
    destPaths.length > 1 &&
    srcPaths.length != destPaths.length
  ) {
    throw new Error(
      'number of source and destination paths must match when more than one destination path is specified'
    );
  }

  return { srcPaths, destPaths };
}

/**
 * Copy the given source path(s) to the given destination path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
export async function copy(
  src: string | string[],
  dest: string | string[],
  options: FileOpOptions = {}
) {
  const { srcPaths, destPaths } = validateInput('copy', src, dest);
  const { showDialogOnError } = Object.assign(
    { showDialogOnError: true },
    options
  );

  const from = srcPaths.map((p) => '`"' + p + '`"').join(' ');
  const to = destPaths.map((p) => '`"' + p + '`"').join(' ');

  const args = `copy ${
    showDialogOnError ? '--show-errors' : ''
  } --from ${from} --to ${to}`;

  const output = await commandsAsScript(
    `Start-Process -WindowStyle Hidden -FilePath "${exe}" -ArgumentList "${args}"`
  );

  return output.exitCode;
}

/**
 * Move the given source path(s) to the given destination path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
export async function move(
  src: string | string[],
  dest: string | string[],
  options: FileOpOptions = {}
) {
  const { srcPaths, destPaths } = validateInput('move', src, dest);
  const { showDialogOnError } = Object.assign(
    { showDialogOnError: true },
    options
  );

  const from = srcPaths.map((p) => '`"' + p + '`"').join(' ');
  const to = destPaths.map((p) => '`"' + p + '`"').join(' ');

  const args = `move ${
    showDialogOnError ? '--show-errors' : ''
  } --from ${from} --to ${to}`;

  const output = await commandsAsScript(
    `Start-Process -WindowStyle Hidden -FilePath "${exe}" -ArgumentList "${args}"`
  );

  return output.exitCode;
}

/**
 * Delete the given source path(s). All paths should be absolute.
 * Returns the exit code of the launcher process (not the launched explorer process).
 * @throws Throws on invalid input
 */
export async function del(src: string | string[], options: FileOpOptions = {}) {
  const { srcPaths } = validateInput('delete', src, []);
  const { showDialogOnError } = Object.assign(
    { showDialogOnError: true },
    options
  );

  const from = srcPaths.map((p) => '`"' + p + '`"').join(' ');

  const args = `delete ${
    showDialogOnError ? '--show-errors' : ''
  } --from ${from}`;

  const output = await commandsAsScript(
    `Start-Process -WindowStyle Hidden -FilePath "${exe}" -ArgumentList "${args}"`
  );

  return output.exitCode;
}

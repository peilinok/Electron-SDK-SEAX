const shell = require('shelljs');

const getPlatform = () => {
  if (process.platform === 'darwin') {
    return 'mac';
  }
  if (process.platform === 'win32') {
    return 'win';
  }
  return 'unsupported';
};

const install = () => {
  let platform = getPlatform();
  if (platform === 'mac') {
    shell.echo('Building AgoraRTC SDK for mac, this will cost a little time...');
    if (shell.exec('node-gyp rebuild', { silent: true }).code !== 0) {
      shell.echo('Building AgoraRTC SDK for mac failed.');
    }
  } else if (platform === 'win') {
    shell.echo(
      'Building AgoraRTC SDK for windows 32bit, this will cost a little time...'
    );
    if (
<<<<<<< HEAD
      shell.exec('node-gyp rebuild --arch=ia32 --msvs_version=2015 --target=1.8.3 --dist-url=https://atom.io/download/electron', { silent: true })
=======
      shell.exec('node-gyp rebuild --arch=ia32', { silent: true })
>>>>>>> fixed script
        .code !== 0
    ) {
      shell.echo('Building AgoraRTC SDK for window 32bit failed.');
    }
  } else {
    shell.echo('Sorry, this sdk only provide win32 and mac version.');
    shell.exit(1);
  }
};

install();

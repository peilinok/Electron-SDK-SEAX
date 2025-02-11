const {logger} = require('just-task');
const shell = require("shelljs");
const path = require('path')
const fs = require('fs')

// workaround to find executable when install as dependency
let gypPath = require.resolve(path.join('node-gyp', 'bin', 'node-gyp.js'));

if (!fs.existsSync(gypPath)) {
  logger.info(`gyp_exec not found at ${gypPath}, switch`);
  gypPath = `${path.resolve(
    __dirname,
    '../node_modules/node-gyp/bin/node-gyp.js'
  )}`;
}

const gyp_exec = `node ${gypPath}`
const agora_node_ext_path = `${path.resolve(__dirname, '../build/Release/agora_node_ext.node')}`
const video_source_path = `${path.resolve(__dirname, '../build/Release/VideoSource')}`

module.exports = ({
  electronVersion='12.0.0',
  runtime='electron',
  platform=process.platform,
  packageVersion,
  debug = false,
  silent = false,
  msvsVersion = '2019',
  arch = 'ia32',
  distUrl = 'https://electronjs.org/headers',
  cb = () => {}
}) => {
  /** get command string */
  const command = [`${gyp_exec} configure`];
  console.log(`Agora VS:${msvsVersion}`)
  // check platform
  if (platform === 'win32') {
    command.push(`--arch=${arch} --msvs_version=${msvsVersion}`)
  }
  if (platform === "darwin" && arch === 'arm64') {
    command.push('--arch=arm64')
  }
  // check runtime
  if (runtime === 'electron') {
    command.push(`--target=${electronVersion} --dist-url=${distUrl}`)
  }

  // check debug
  if (debug) {
    command.push('--debug');
    if (platform === 'darwin') {
      // MUST AT THE END OF THE COMMAND ARR
      command.push('-- -f xcode')
    }
  }

  const commandStr = command.join(' ')

  /** start build */
  logger.info(commandStr, '\n');

  logger.info("Package Version:", packageVersion);
  logger.info("Platform:", platform);
  logger.info("Electron Version:", electronVersion);
  logger.info("Runtime:", runtime, "\n");

  logger.info("Build C++ addon for Agora Electron SDK...\n")

  shell.exec(`${gyp_exec} clean`, {silent}, (code, stdout, stderr) => {
    // handle error
    logger.info(`clean done ${stdout}`)
    if (code !== 0) {
      logger.error(stderr);
      process.exit(1)
    }

    shell.exec(commandStr, {silent}, (code, stdout, stderr) => {
      // handle error
      logger.info(`configure done ${stdout}`)
      if (code !== 0) {
        logger.error(stderr);
        process.exit(1)
      }

      if (debug) {
        // handle success
        logger.info('Complete, please go to `/build` and build manually')
        process.exit(0)
      } else {
        shell.exec(`${gyp_exec} build`, {silent}, (code, stdout, stderr) => {
          // handle error
          if (code !== 0) {
            logger.error(stderr);
            process.exit(1)
          }
          logger.info('Build complete')
          cb(true);
        })
      }
    })
  })
}

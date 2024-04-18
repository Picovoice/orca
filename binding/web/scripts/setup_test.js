const fs = require('fs');
const { join } = require('path');

console.log('Copying the orca & leopard models...');

const testDirectory = join(__dirname, '..', 'test');
const fixturesDirectory = join(__dirname, '..', 'cypress', 'fixtures');

const paramsSourceDirectory = join(
  __dirname,
  '..',
  '..',
  '..',
  'lib',
  'common',
);

const sourceDirectory = join(
  __dirname,
  '..',
  '..',
  '..',
  'resources',
);

try {
  fs.mkdirSync(testDirectory, { recursive: true });

  fs.readdirSync(paramsSourceDirectory).forEach(file => {
    fs.copyFileSync(join(paramsSourceDirectory, file), join(testDirectory, file));
  });

  fs.mkdirSync(join(fixturesDirectory, '.test'), { recursive: true });
  fs.copyFileSync(join(sourceDirectory, '.test', 'test_data.json'), join(fixturesDirectory, '.test', 'test_data.json'));
} catch (error) {
  console.error(error);
}

console.log('... Done!');

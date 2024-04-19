const fs = require('fs');
const { join } = require('path');

console.log('Copying the orca & leopard models...');

const testDirectory = join(__dirname, '..', 'test');
const fixturesDirectory = join(__dirname, '..', 'cypress', 'fixtures', 'resources');

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
  '.test',
);

try {
  fs.mkdirSync(testDirectory, { recursive: true });

  fs.readdirSync(paramsSourceDirectory).forEach(file => {
    fs.copyFileSync(join(paramsSourceDirectory, file), join(testDirectory, file));
  });

  fs.mkdirSync(join(fixturesDirectory, '.test', 'wav'), { recursive: true });
  fs.copyFileSync(join(sourceDirectory, 'test_data.json'), join(fixturesDirectory, '.test', 'test_data.json'));

  fs.readdirSync(join(sourceDirectory, 'wav')).forEach(file => {
    fs.copyFileSync(join(sourceDirectory, 'wav', file), join(fixturesDirectory, '.test', 'wav', file));
  });
} catch (error) {
  console.error(error);
}

console.log('... Done!');

if [ ! -d "./orca-test-app/src/androidTest/assets/test_resources/model_files" ]
then
    echo "Creating test model files directory..."
    mkdir -p ./orca-test-app/src/androidTest/assets/test_resources/model_files
fi

echo "Copying orca model files ..."
cp ../../../lib/common/*.pv ./orca-test-app/src/androidTest/assets/test_resources/model_files

echo "Copying test data file..."
cp ../../../resources/.test/test_data.json ./orca-test-app/src/androidTest/assets/test_resources

if [ ! -d "./orca-test-app/src/androidTest/assets/test_resources/wav" ]
then
    echo "Creating test model files directory..."
    mkdir -p ./orca-test-app/src/androidTest/assets/test_resources/wav
fi

echo "Copying test wav files..."
cp ../../../resources/.test/wav/*.wav ./orca-test-app/src/androidTest/assets/test_resources/wav

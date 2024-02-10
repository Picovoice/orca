if [ ! -d "./orca-test-app/src/androidTest/assets/test_resources/model_files" ]
then
    echo "Creating test model files directory..."
    mkdir -p ./orca-test-app/src/androidTest/assets/test_resources/model_files
fi

echo "Copying orca model files ..."
cp ../../../lib/common/*.pv ./orca-test-app/src/androidTest/assets/test_resources/model_files

echo "Copying test data file..."
cp ../../../resources/.test/test_data.json ./orca-test-app/src/androidTest/assets/test_resources
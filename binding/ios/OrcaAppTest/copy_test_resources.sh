LIB_DIR="../../../lib"
RESOURCE_DIR="../../../resources"
ASSETS_DIR="./test_resources"

echo "Copying test model files..."
mkdir -p ${ASSETS_DIR}/model_files
cp ${LIB_DIR}/common/*.pv ${ASSETS_DIR}/model_files

echo "Copying wav files..."
mkdir -p ${ASSETS_DIR}/wav
cp -r ${RESOURCE_DIR}/.test/wav/mac-*/ ${ASSETS_DIR}/wav/

echo "Copying test data file..."
cp ${RESOURCE_DIR}/.test/mac-*_test_data.json ${ASSETS_DIR}/

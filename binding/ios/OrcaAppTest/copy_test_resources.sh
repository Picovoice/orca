LIB_DIR="../../../lib"
RESOURCE_DIR="../../../resources"
ASSETS_DIR="./OrcaAppTestUITests/test_resources"

echo "Copying test model files..."
mkdir -p ${ASSETS_DIR}/model_files
cp ${LIB_DIR}/common/*.pv ${ASSETS_DIR}/model_files

echo "Copying test data file..."
cp ${RESOURCE_DIR}/.test/test_data.json ${ASSETS_DIR}
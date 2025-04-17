const fs = require('fs');
const path = require('path');

function getAvailableLanguages() {
    const testDataPath = path.join(__dirname, '../../resources/.test/test_data.json');
    const fileContent = fs.readFileSync(testDataPath, 'utf8');
    const testData = JSON.parse(fileContent);

    return testData.tests.sentence_tests.map(x => x.language);
}

function getAvailableGenders() {
    return ['male', 'female'];
}

function getModelPath(language, gender) {
    const modelName = `orca_params_${language}_${gender}.pv`;
    const modelPath = path.join(__dirname, `../../lib/common/${modelName}`);

    if (fs.existsSync(modelPath)) {
        return modelPath;
    } else {
        const modelsDir = path.join(__dirname, '../../lib/common');
        const files = fs.readdirSync(modelsDir);
        let availableGender = null;

        for (const filename of files) {
            if (
                filename.startsWith(`orca_params_${language}_`) &&
                fs.statSync(path.join(modelsDir, filename)).isFile()
            ) {
                availableGender = filename.split('.')[0].split('_').pop();
                break;
            }
        }

        throw new Error(
            `Gender '${gender}' is not available with language '${language}'. ` +
            `Please use gender '${availableGender}'.`
        );
    }
}

module.exports = {
    getAvailableLanguages,
    getAvailableGenders,
    getModelPath
};

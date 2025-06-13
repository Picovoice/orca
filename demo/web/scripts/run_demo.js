const child_process = require("child_process");
const fs = require("fs");
const path = require("path");
const testData = require("../../../resources/.test/test_data.json");

const availableLanguages = testData["tests"]["sentence_tests"].map(
  (x) => x["language"],
);

const genders = ["male", "female"];

const language = process.argv.slice(2)[0];
if (!language) {
  console.error(
    `Choose the language you would like to run the demo in with "yarn start [language] [gender]".\nAvailable languages are ${availableLanguages.join(
      ", ",
    )}`,
  );
  process.exit(1);
}

if (!availableLanguages.includes(language)) {
  console.error(
    `'${language}' is not an available demo language.\nAvailable languages are ${availableLanguages.join(
      ", ",
    )}`,
  );
  process.exit(1);
}

const gender = process.argv.slice(2)[1];
if (!gender) {
  console.error(
    `Choose the gender of the synthesized voice with "yarn start [language] [gender]".\nAvailable genders are ${genders.join(
      ", ",
    )}`,
  );
  process.exit(1);
}

if (!gender.includes(gender)) {
  console.error(
    `'${gender}' is not an available gender.\nAvailable genders are ${genders.join(
      ", ",
    )}`,
  );
  process.exit(1);
}

const version = process.env.npm_package_version;
const rootDir = path.join(__dirname, "..", "..", "..");

const modelDir = path.join(rootDir, "lib", "common");

let outputDirectory = path.join(__dirname, "..", "models");
if (fs.existsSync(outputDirectory)) {
  fs.readdirSync(outputDirectory).forEach((k) => {
    fs.unlinkSync(path.join(outputDirectory, k));
  });
} else {
  fs.mkdirSync(outputDirectory, { recursive: true });
}

const modelName = `orca_params_${language}_${gender}.pv`;
const modelPath = path.join(modelDir, modelName);
if (fs.existsSync(modelPath)) {
  fs.copyFileSync(
      path.join(modelDir, modelName),
      path.join(outputDirectory, modelName),
  );
} else {
  let availableGender;

  fs.readdirSync(modelDir).forEach((filename) => {
    if (
        filename.startsWith(`orca_params_${language}_`) &&
        fs.statSync(path.join(modelDir, filename)).isFile()
    ) {
      availableGender = filename.split('.')[0].split('_').pop();
    }
  });

  throw new Error(`Gender '${gender}' is not available with language '${language}'. Please use gender '${availableGender}'`);
}

fs.writeFileSync(
  path.join(outputDirectory, "orcaModel.js"),
  `const orcaModel = {
  publicPath: "models/${modelName}",
  customWritePath: "${version}_${modelName}",
};

(function () {
  if (typeof module !== "undefined" && typeof module.exports !== "undefined")
    module.exports = orcaModel;
})();`,
);

const command = process.platform === "win32" ? "npx.cmd" : "npx";

child_process.execSync(`${command} http-server -c-1 -a localhost -p 5000`, {
  shell: true,
  stdio: "inherit",
});

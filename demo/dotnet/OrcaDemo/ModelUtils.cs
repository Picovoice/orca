using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;

public class ModelUtils
{
    private static readonly string ROOT_DIR = Path.Combine(
        AppContext.BaseDirectory,
        "../../../../../..");

    private static string GetPlatformName()
    {
        string platformName = Environment.GetEnvironmentVariable("PLATFORM_NAME");
        if (platformName == null)
        {
            Console.WriteLine("Expected PLATFORM_NAME to exist. Is this being run in a pipeline?");
            Environment.Exit(1);
        }

        if (platformName == "ios")
        {
            platformName = "mac";
        }

        return platformName;
    }

    private static string GetArchitecture()
    {
        string platformName = GetPlatformName();
        string architecture = RuntimeInformation.OSArchitecture.ToString();

        if (platformName == "windows" && architecture == "X64")
        {
            architecture = "AMD64";
        }
        else if (platformName == "windows" && architecture == "Arm64")
        {
            architecture = "ARM64";
        }
        else if (architecture == "X64")
        {
            architecture = "x86_64";
        }
        else if (platformName == "mac" && architecture == "Arm64")
        {
            architecture = "arm64";
        }
        else if (architecture == "Arm64")
        {
            architecture = "aarch64";
        }

        return architecture;
    }

    public static string GetDataFilePath() {
        string platformName = GetPlatformName();
        string architecture = GetArchitecture();
        string dataFilePath = Path.Combine(
                ROOT_DIR,
                $"resources/.test/{platformName}-{architecture}_test_data.json");

        if (File.Exists(dataFilePath))
            return dataFilePath;

        string resourcesDir = Path.Combine(ROOT_DIR, "resources/.test/");
        if (!Directory.Exists(resourcesDir))
            throw new Exception("Could not find ./.test/ directory");

        foreach (var file in Directory.EnumerateFiles(resourcesDir))
        {
            string name = Path.GetFileName(file);
            if (name.StartsWith($"{platformName}-", StringComparison.Ordinal) &&
                name.EndsWith(".json", StringComparison.Ordinal))
            {
                return Path.Combine(
                        ROOT_DIR,
                        $"resources/.test/{name}");
            }
        }

        throw new Exception("Could not find any test_data for the current platform");
    }

    public static List<string> GetAvailableLanguages()
    {
        string testDataPath = GetDataFilePath();
        testDataPath = Path.GetFullPath(testDataPath);

        string jsonString = File.ReadAllText(testDataPath);
        using JsonDocument document = JsonDocument.Parse(jsonString);

        JsonElement sentenceTests = document.RootElement
            .GetProperty("tests")
            .GetProperty("sentence_tests");

        List<string> languages = new List<string>();

        foreach (JsonElement item in sentenceTests.EnumerateArray())
        {
            languages.Add(item.GetProperty("language").GetString());
        }

        return languages;
    }

    public static List<string> GetAvailableGenders()
    {
        return new List<string> { "male", "female" };
    }

    public static string GetModelPath(string language, string gender)
    {
        string modelName = $"orca_params_{language}_{gender}.pv";
        string modelPath = Path.Combine(ROOT_DIR, $"lib/common/{modelName}");
        modelPath = Path.GetFullPath(modelPath);

        if (File.Exists(modelPath))
        {
            return modelPath;
        }
        else
        {
            string modelsDir = Path.GetFullPath(Path.Combine(ROOT_DIR, "lib/common"));
            string[] files = Directory.GetFiles(modelsDir);
            string availableGender = null;

            foreach (string file in files)
            {
                string filename = Path.GetFileName(file);
                if (filename.StartsWith($"orca_params_{language}_") && File.Exists(file))
                {
                    string[] parts = Path.GetFileNameWithoutExtension(filename).Split('_');
                    availableGender = parts.Last();
                    break;
                }
            }

            throw new ArgumentException($"Gender '{gender}' is not available with language '{language}'. " +
                                        $"Please use gender '{availableGender}'.");
        }
    }
}
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

public class ModelUtils
{
    private static readonly string ROOT_DIR = Path.Combine(
        AppContext.BaseDirectory,
        "../../../../../..");

    public static List<string> GetAvailableLanguages()
    {
        string testDataPath = Path.Combine(ROOT_DIR, "resources/.test/test_data.json");
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

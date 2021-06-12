using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using PuzzleCommon;
using Xunit;

namespace PuzzleTests
{
    public class GenerationTests
    {
        [Theory, MemberData(nameof(PuzzleNames))]
        public async Task TestPreset(string puzzle, int index, string name)
        {
            var stub = InterfaceStubs.Instance;
            var fe = new WindowsModern(puzzle, stub, stub, stub);

            // TODO when an error occurs, the entire program aborts instead of throwing an exception
            fe.SetPreset(index);
            await fe.NewGame();

            Assert.Equal(name, fe.GetPresetList(false, 1).GetCurrentPresetName());
        }

        public static IEnumerable<object[]> PuzzleNames()
        {
            var list = WindowsModern.GetPuzzleList();

            foreach (var puzzle in list.Items)
            {
                var stub = InterfaceStubs.Instance;
                var fe = new WindowsModern(puzzle.Name, stub, stub, stub);

                var presets = fe.GetPresetList(false, 1);
                foreach (var preset in presets.Items)
                    yield return new object[] { puzzle.Name, preset.Index, preset.Name};
            }
        }
    }
}

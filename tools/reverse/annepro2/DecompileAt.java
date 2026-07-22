// Ghidra headless helper for the raw AnnePro2 firmware images.
// @category AnnePro2

import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DecompileAt extends GhidraScript {
    @Override
    protected void run() throws Exception {
        Listing listing = currentProgram.getListing();
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);

        for (String argument : getScriptArgs()) {
            long offset = Long.decode(argument);
            Address entry = toAddr(offset);
            disassemble(entry);

            Function function = listing.getFunctionAt(entry);
            if (function == null) {
                function = createFunction(entry, "FUN_" + Long.toHexString(offset));
            }

            println("\n=== " + entry + " " + (function == null ? "<no function>" : function.getName()) + " ===");
            Instruction instruction = listing.getInstructionAt(entry);
            for (int count = 0; instruction != null && count < 80; count++) {
                println(instruction.getAddress() + "  " + instruction);
                instruction = instruction.getNext();
            }

            if (function != null) {
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                if (result.decompileCompleted()) {
                    println(result.getDecompiledFunction().getC());
                } else {
                    println("Decompiler error: " + result.getErrorMessage());
                }
            }
        }
        decompiler.dispose();
    }
}

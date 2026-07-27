// Find functions in a raw firmware image that use all requested scalar values.
// @category AnnePro2

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class FindFunctionsWithScalars extends GhidraScript {
    @Override
    protected void run() throws Exception {
        List<Long> requested = new ArrayList<>();
        for (String argument : getScriptArgs()) {
            requested.add(Long.decode(argument));
        }
        if (requested.isEmpty()) {
            throw new IllegalArgumentException("pass at least one scalar value");
        }

        FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
        while (functions.hasNext() && !monitor.isCancelled()) {
            Function function = functions.next();
            Map<Long, List<Instruction>> matches = new LinkedHashMap<>();
            for (Long value : requested) {
                matches.put(value, new ArrayList<>());
            }

            InstructionIterator instructions =
                currentProgram.getListing().getInstructions(function.getBody(), true);
            while (instructions.hasNext()) {
                Instruction instruction = instructions.next();
                for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
                    for (Object object : instruction.getOpObjects(operand)) {
                        if (object instanceof Scalar) {
                            long value = ((Scalar)object).getUnsignedValue();
                            List<Instruction> found = matches.get(value);
                            if (found != null) {
                                found.add(instruction);
                            }
                        }
                    }
                }
            }

            boolean complete = true;
            for (Long value : requested) {
                if (matches.get(value).isEmpty()) {
                    complete = false;
                    break;
                }
            }
            if (!complete) {
                continue;
            }

            println("\n=== " + function.getEntryPoint() + " " + function.getName() + " ===");
            for (Long value : requested) {
                for (Instruction instruction : matches.get(value)) {
                    println(String.format("scalar 0x%x: %s  %s",
                        value, instruction.getAddress(), instruction));
                }
            }
        }
    }
}

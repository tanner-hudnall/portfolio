import java.io.File;
import java.util.ArrayList;
import java.util.Scanner;

public class Driver {
    public static String filename;
    public static boolean testResponseTime;
    public static boolean testPoliceStationPositions;

    public static void main(String[] args) throws Exception {
        parseArgs(args);

        TownPlan problem = parseTownPlanProblem(filename);
        testRun(problem);
    }

    private static void usage() {
        System.err.println("usage: java Driver [-r] [-p] <filename>");
        System.err.println("\t-r\tTest optimal response time");
        System.err.println("\t-p\tTest positions of police stations");
        System.exit(1);
    }

    public static void parseArgs(String[] args) {
        if (args.length == 0) {
            usage();
        }

        filename = "";
        testResponseTime = false;
        testPoliceStationPositions = false;
        boolean flagsPresent = false;

        for (String s : args) {
            if (s.equals("-r")) {
                flagsPresent = true;
                testResponseTime = true;
            } else if (s.equals("-p")) {
                flagsPresent = true;
                testPoliceStationPositions = true;
            } else if (!s.startsWith("-")) {
                filename = s;
            } else {
                System.err.printf("Unknown option: %s\n", s);
                usage();
            }
        }

        if (!flagsPresent) {
            testResponseTime = true;
            testPoliceStationPositions = true;
        }
    }

    public static TownPlan parseTownPlanProblem(String inputFile) throws Exception {
        int n = 0;
        int k = 0;
        ArrayList<Integer> position_houses;

        Scanner sc = new Scanner(new File(inputFile));
        String[] inputSizes = sc.nextLine().split(" ");

        n = Integer.parseInt(inputSizes[0]);
        k = Integer.parseInt(inputSizes[1]);
        position_houses = readPositionsList(sc, n);

        TownPlan problem = new TownPlan(n, k, position_houses);

        return problem;
    }

    private static ArrayList<Integer> readPositionsList(Scanner sc, int n) {
        ArrayList<Integer> position_houses = new ArrayList<Integer>(0);

        String[] pos = sc.nextLine().split(" ");
        for (int i = 0; i < n; i++) {
            position_houses.add(Integer.parseInt(pos[i]));
        }

        return position_houses;
    }

    public static void testRun(TownPlan problem) {
        Program3 program = new Program3();

        if (testResponseTime) {
	        problem.setResponseTime(null);
            problem.setPoliceStationPositions(null);
            TownPlan responseTownPlan = program.findOptimalResponseTime(problem);
            System.out.println(responseTownPlan);
        }

        if (testPoliceStationPositions) {
	        problem.setResponseTime(null);
	        problem.setPoliceStationPositions(null);
            TownPlan posPoliceTownPlan = program.findOptimalPoliceStationPositions(problem);
            System.out.println(posPoliceTownPlan);
        }
    }
}

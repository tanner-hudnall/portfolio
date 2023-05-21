import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.api.java.JavaPairRDD;
import scala.Tuple2;

import java.util.*;
import java.io.*;

public class WordGraph {
    public static void main(String[] args) {
        SparkConf conf = new SparkConf().setAppName("WordGraphSpark");
        JavaSparkContext sc = new JavaSparkContext(conf);
        JavaRDD<String> rdd = sc.textFile(args[0]);
        JavaRDD<String> altered_rdd = input_processor(rdd);
        JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> wordGraph = buildWordGraph(sc, altered_rdd);
        JavaPairRDD<String, Tuple2<List<String>, Map<String, Double>>> weightedMap = computeWeights(sc, wordGraph);
        // printer(rdd, altered_rdd, wordGraph, weightedMap);
        outputCreator(weightedMap, args[1]);
        // Stop the JavaSparkContext object

        sc.stop();
        sc.close();
    }

    public static JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> buildWordGraph(JavaSparkContext sc,
            JavaRDD<String> lines) {
        JavaPairRDD<Tuple2<String, String>, Integer> wordPairs = lines.flatMapToPair(line -> {
            String[] words = line.split("\\s+");
            List<Tuple2<Tuple2<String, String>, Integer>> pairs = new ArrayList<>();
            for (int i = 0; i < words.length - 1; i++) {
                String predecessor = words[i].toLowerCase();
                String successor = words[i + 1].toLowerCase();
                // if (i == words.length - 2 && line.endsWith(successor)) {
                // continue;
                // }
                pairs.add(new Tuple2<>(new Tuple2<>(predecessor, successor), 1));
            }
            return pairs.iterator();
        });

        JavaPairRDD<Tuple2<String, String>, Integer> wordCounts = wordPairs.reduceByKey(Integer::sum);

        JavaPairRDD<String, Iterable<Tuple2<String, Integer>>> wordGroups = wordCounts.mapToPair(pair -> {
            Tuple2<String, String> key = pair._1();
            Integer count = pair._2();
            return new Tuple2<>(key._1(), new Tuple2<>(key._2(), count));
        }).groupByKey();

        JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> wordGraph = wordGroups.mapValues(iter -> {
            Map<String, Integer> map = new HashMap<>();
            List<String> successors = new ArrayList<>();
            for (Tuple2<String, Integer> pair : iter) {
                successors.add(pair._1());
                map.put(pair._1(), pair._2());
            }
            return new Tuple2<>(successors, map);
        });

        return wordGraph;
    }

    public static JavaRDD<String> input_processor(JavaRDD<String> lines) {
        JavaRDD<String> processedLines = lines.map(line -> {
            String lowerCaseStr = line.toLowerCase();
            String maskedStr = lowerCaseStr.replaceAll("[^a-z0-9]+", " ");
            String[] words = maskedStr.split("\\s+");
            String processedStr = String.join(" ", words);
            return processedStr;
        });
        return processedLines;
    }

    public static void printer(JavaRDD<String> rdd, JavaRDD<String> altered_rdd,
            JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> wordGraph,
            JavaPairRDD<String, Tuple2<List<String>, Map<String, Double>>> weightedMap) {
        List<String> list_1 = rdd.collect();
        for (String s : list_1) {
            System.out.println(s);
        }

        System.out.println("\n\ninto\n\n");

        List<String> list_2 = altered_rdd.collect();
        for (String s : list_2) {
            System.out.println(s);
        }

        System.out.println("\n\nmaps to\n\n");

        List<Tuple2<String, Tuple2<List<String>, Map<String, Integer>>>> output_1 = wordGraph.collect();
        for (Tuple2<String, Tuple2<List<String>, Map<String, Integer>>> tuple : output_1) {
            System.out.println(tuple._1() + ": " + tuple._2()._2() + ", Successors: " + tuple._2()._1());
        }

        System.out.println("\n\nwith weights\n\n");
        List<Tuple2<String, Tuple2<List<String>, Map<String, Double>>>> output_2 = weightedMap.collect();
        for (Tuple2<String, Tuple2<List<String>, Map<String, Double>>> tuple : output_2) {
            System.out.println(tuple._1() + ": " + tuple._2()._2() + ", Successors: " + tuple._2()._1());
        }

    }

    public static void printer(JavaRDD<String> rdd, JavaRDD<String> altered_rdd,
            JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> wordGraph) {
        List<String> list_1 = rdd.collect();
        for (String s : list_1) {
            System.out.println(s);
        }

        System.out.println("\n\ninto\n\n");

        List<String> list_2 = altered_rdd.collect();
        for (String s : list_2) {
            System.out.println(s);
        }

        System.out.println("\n\nmaps to\n\n");

        List<Tuple2<String, Tuple2<List<String>, Map<String, Integer>>>> output = wordGraph.collect();
        for (Tuple2<String, Tuple2<List<String>, Map<String, Integer>>> tuple : output) {
            System.out.println(tuple._1() + ": " + tuple._2()._2() + ", Successors: " + tuple._2()._1());
        }

    }

    public static JavaPairRDD<String, Tuple2<List<String>, Map<String, Double>>> computeWeights(JavaSparkContext sc,
            JavaPairRDD<String, Tuple2<List<String>, Map<String, Integer>>> wordGraph) {
        JavaPairRDD<String, Tuple2<List<String>, Map<String, Double>>> weightsRDD = wordGraph.mapValues(value -> {
            Map<String, Double> weightsMap = new HashMap<>();
            int totalCount = value._2().values().stream().reduce(0, Integer::sum);
            value._2().forEach((neighbor, count) -> {
                double weight = (double) count / (double) totalCount;
                weightsMap.put(neighbor, weight);
            });
            return new Tuple2<>(value._1(), weightsMap);
        });
        return weightsRDD;
    }

    public static void outputCreator(JavaPairRDD<String, Tuple2<List<String>, Map<String, Double>>> word_graph,
            String path) {
        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(path));
            List<Tuple2<String, Tuple2<List<String>, Map<String, Double>>>> wordGraphList = word_graph.collect();

            for (Tuple2<String, Tuple2<List<String>, Map<String, Double>>> node : wordGraphList) {
                String predecessor = node._1();
                Tuple2<List<String>, Map<String, Double>> successors = node._2();
                int num_successors = successors._1().size();
                writer.write(predecessor + " " + num_successors + "\n");
                for (String successor : successors._2().keySet()) {
                    double weight = successors._2().get(successor);
                    writer.write("<" + successor + ", " + weight + ">\n");
                }
            }
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
package netjson

class Test {
    fun test() {
        val fileContent = Test::class.java.getResource("/examples/NetworkGraph.json").readText()

        Formatter.fromJSON(fileContent);

    }
}
    fun main(args: Array<String>) {
        Test().test();
    }

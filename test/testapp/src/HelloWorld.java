import java.util.*;

public class HelloWorld {

	public static void runRegexTest() {
		StringBuilder sb = new StringBuilder();
		Random rand = new Random();
		for(int i = 0; i < 1024*1024; i++) {
			sb.append((char) 'A' + rand.nextInt('Z' - 'A'));
		}
		String text = sb.toString();
		for(int i = 0; i < 1000; i++) {
			sb = new StringBuilder();
			for(int j = 0; j < 128; j++) {
				sb.append((char) 'A' + rand.nextInt('Z' - 'A'));
			}
			String search = sb.toString();
			text.contains(search);
		}
	}

	public static void main(String[] args) {
		System.out.println("Hello World");
		runRegexTest();
	}
}

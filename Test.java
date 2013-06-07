/*
 * Test.java
 * Braden Simpson (V00685500)
 * Jordan Ell (V00660306)
 * University of Victoria, CSC586A
 * Virtual Machines
 */
public class Test {
	public static int returnX() {
		int x = 1;
                if(x > 0) {
                    int y = 0;
                    y = y + 1;
                    x = 2;
                }
		return x + 309;
	}
}
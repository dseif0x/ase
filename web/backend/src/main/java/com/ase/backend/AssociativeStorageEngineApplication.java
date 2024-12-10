package com.ase.backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class AssociativeStorageEngineApplication {

	/**
	 * Starting point of the backend.
	 *
	 */
	public static void main(String[] args) {
		SpringApplication.run(AssociativeStorageEngineApplication.class, args);
	}

	/**
	 * Multiply a and b and returns the value.
	 *
	 * @param a Int to multiply
	 * @param b Int to multiply
	 */
	public static int testMul(int a, int b) {
		return a * b;
	}
}

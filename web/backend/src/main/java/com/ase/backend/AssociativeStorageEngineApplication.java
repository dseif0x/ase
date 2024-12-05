package com.ase.backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class AssociativeStorageEngineApplication {

	public static void main(String[] args) {
		SpringApplication.run(AssociativeStorageEngineApplication.class, args);
	}

	// this is just an example function for the junit integration
	public static int testMul(int a, int b) {
		return a * b;
	}
}

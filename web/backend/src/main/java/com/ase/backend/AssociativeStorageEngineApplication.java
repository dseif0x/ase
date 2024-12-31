package com.ase.backend;

import com.ase.driver.DatabaseClient;
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
}

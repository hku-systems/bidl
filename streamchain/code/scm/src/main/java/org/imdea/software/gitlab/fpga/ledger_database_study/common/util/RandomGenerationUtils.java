package org.imdea.software.gitlab.fpga.ledger_database_study.common.util;

import java.util.Random;
import java.util.function.DoubleFunction;

public class RandomGenerationUtils {

	public static RandomGenerationUtils _instance;
	
	private final long seed;
	private final Random random;
	
	private RandomGenerationUtils(long seed) {
			this.seed = seed;
			this.random = new Random(seed);
	}
	
	public static RandomGenerationUtils getInstance(long seed) {
		if( _instance == null ) {
			_instance = new RandomGenerationUtils(seed);
		}
		
		return _instance;
	}
	
	public float getRandomFloat() {
		return this.random.nextFloat();
	}
	
	public double getRandomDouble() {
		return this.random.nextDouble();
	}
	public long getSeed() {
		return this.seed;
	}
	
	public int getOperation(int numberOfOperations, DoubleFunction<Integer> probabilityClassifier ) {
		return probabilityClassifier.apply(this.getRandomFloat());
	}
	
	public int getInteger(int start, int bound) {
//		r.nextInt((max - min) + 1) + min
		return this.random.nextInt((bound-start) + 1) + start;
	}
}
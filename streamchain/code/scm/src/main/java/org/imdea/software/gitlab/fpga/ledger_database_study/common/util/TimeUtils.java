package org.imdea.software.gitlab.fpga.ledger_database_study.common.util;

import java.sql.Date;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class TimeUtils {

	private static long currTime = new java.util.Date().getTime();
	
	private static long timeIncrement = 1000 * 60 * 60;
	
	private static SimpleDateFormat timeFormat = 
			new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
	
	private static SimpleDateFormat timeFabricFormat = 
			new SimpleDateFormat("yyyy-MM-dd%HH:mm:ss");
	
	private static SimpleDateFormat dateFormat = 
			new SimpleDateFormat("yyyy-MM-dd");
	
	public static long getNanoTime() {
		return System.nanoTime();
	}
	
	public static java.util.Date getSimulatedDate() {
		currTime += timeIncrement;
		long simulatedTime = new Long(currTime);
		return new java.util.Date(simulatedTime);
	}
	
	public static java.util.Date getSimulatedYearEnd(int noOfYears) {
		
		long simulatedYearEnd = currTime + timeIncrement * 24 * 365 * noOfYears;
		return new java.util.Date(simulatedYearEnd);
	}
	
	public static String getDateTimeString(int day, int month, int year, int hours, int minutes, int seconds) {
		Calendar cal = Calendar.getInstance();
		
		cal.clear();
		cal.set(year, (month - 1), day, hours, minutes, seconds);
		
		return (timeFormat.format(cal.getTime()));
	}
	
	public static String getFabricDateTimeString(int day, int month, int year, int hours, int minutes, int seconds) {
		Calendar cal = Calendar.getInstance();
		
		cal.clear();
		cal.set(year, (month - 1), day, hours, minutes, seconds);
		
		return (timeFabricFormat.format(cal.getTime()));
	}
	
	public static Date getSQLDatestamp(int day, int month, int year) {
		
		Calendar cal = Calendar.getInstance();
		cal.clear();
//		cal.setTimeZone(TimeZone.getTimeZone("GMT"));
		cal.set(year, (month - 1), day);
		
		return new java.sql.Date(cal.getTimeInMillis());
	}
	
	public static Timestamp getSQLTimestamp(int day, int month, int year, int hours, int mins, int secs) {

		Calendar cal = Calendar.getInstance();
		
		cal.clear();
//		cal.setTimeZone(TimeZone.getTimeZone("GMT"));
		cal.set(year, (month - 1), day, hours, mins, secs);
		
		return new java.sql.Timestamp(cal.getTimeInMillis());
	}
	
	public static String formatSQLTime(Timestamp time) {
		
		
//		format.setTimeZone(T imeZone.getTimeZone("GMT"));
		
		return (timeFormat.format((java.util.Date)time));
	}
	
	public static String formatSQLDate(java.sql.Date date) {
//		format.setTimeZone(TimeZone.getTimeZone("GMT"));
		
		return (dateFormat.format(date));
	}
}
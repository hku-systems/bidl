package org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.common;

import java.io.BufferedWriter;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;

public abstract class FilesystemReport<T> implements Logger<T>{

	private static String resultsFolder = null;
	
	protected FilesystemReport() throws Exception{
		if( resultsFolder == null ) {
			createFolderIfNotExists(true);
		}
	}
	
	protected FilesystemReport(String resultsFolder) throws Exception{
		if( resultsFolder != null ) {
			this.resultsFolder = resultsFolder;
		}
		
		createFolderIfNotExists(false);
	}
	
	protected BufferedWriter getWriter(String fileName,boolean isAppend) throws Exception {
		String filePath = resultsFolder + "/" + fileName;
		StandardOpenOption FileTruncateOption = isAppend 
				? StandardOpenOption.APPEND 
						: StandardOpenOption.TRUNCATE_EXISTING;
		
		BufferedWriter writer = Files.newBufferedWriter(Paths.get(filePath),StandardOpenOption.CREATE,FileTruncateOption);
		return writer;
	}
	
	private void createFolderIfNotExists(boolean withDate) throws Exception {
		Date date = new Date();
		
		if( withDate) {
			SimpleDateFormat format = new SimpleDateFormat("YYYY-MM-dd-HH-mm-ss");
			resultsFolder = Configuration.reportBaseFolder + "/"+ format.format(date);
		}
			
		createDirectory(resultsFolder);
	}
	
	private void createDirectory(String folderPath) throws Exception {
		
		Path directoryObject = Paths.get(folderPath);
        boolean isDirectoryPresent = Files.exists(directoryObject);
        
        if(isDirectoryPresent) {
            return;
        } 
        
        Files.createDirectories(directoryObject);
	}
}
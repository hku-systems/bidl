package org.imdea.software.gitlab.fpga.ledger_database_study.common.sql;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.TimeZone;

import org.apache.commons.dbcp2.BasicDataSource;
import org.apache.commons.dbcp2.BasicDataSourceFactory;
import org.apache.commons.dbcp2.ConnectionFactory;
import org.apache.commons.dbcp2.DriverManagerConnectionFactory;
import org.apache.commons.dbcp2.PoolableConnection;
import org.apache.commons.dbcp2.PoolableConnectionFactory;
import org.apache.commons.dbcp2.PoolingDataSource;
import org.apache.commons.pool2.ObjectPool;
import org.apache.commons.pool2.impl.GenericObjectPool;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.exception.CustomException;


public class SQLConnection {

	private static SQLConnection connectionInstance;
	
	private BasicDataSource ds = null;
	 
	private SQLConnection() throws CustomException, Exception {
		
		if( ds != null ) {
			return;
		}
		
		String serverName = Configuration.mysqlHostName;
		String portNumber = Configuration.mysqlPort;

		try {
			
//			ConnectionFactory connectionFactory =
//					new DriverManagerConnectionFactory("jdbc:mysql://" + serverName +":" + portNumber + "/SupplyChain-SQL-V1",Configuration.mysqlUserName,Configuration.mysqlPassword);
//			
//			PoolableConnectionFactory poolableConnectionFactory =
//					 new PoolableConnectionFactory(connectionFactory, null);
//			
//			ObjectPool<PoolableConnection> connectionPool =
//					 new Obj
//			
//			poolableConnectionFactory.setPoolStatements(true);
//			poolableConnectionFactory.setPool(connectionPool);
//			
//			poolableConnectionFactory.setMaxOpenPreparedStatements(100);
//			PoolingDataSource<PoolableConnection> dataSource =
//					 new Pooling DataSource<>(connectionPool);
			ds = new BasicDataSource();
			ds.setMaxTotal(500);
			ds.setInitialSize(110);
			ds.setAccessToUnderlyingConnectionAllowed(true);
	        ds.setMaxOpenPreparedStatements(100200);
	        ds.setPoolPreparedStatements(true);
			ds.setMinIdle(200);
	        ds.setMaxIdle(200);
			ds.setDriverClassName("com.mysql.cj.jdbc.Driver");
			ds.setUrl("jdbc:mysql://" + serverName +":" + portNumber + "/SupplyChain-SQL-V1?serverTimezone=" + TimeZone.getDefault().getID());
	        ds.setUsername(Configuration.mysqlUserName);
	        ds.setPassword(Configuration.mysqlPassword);
//	        
		}catch( Exception e) {
			throw e;
		}
	}
 
	public Connection getConnection() throws SQLException {
//		Connection connection = ds.getConnection();
//		connection.setTransactionIsolation(Connection.TRANSACTION_REPEATABLE_READ);
	    return ds.getConnection();
	}
	
	public static SQLConnection getInstance() throws Exception {
		
		if( connectionInstance == null ) {
			connectionInstance = new SQLConnection();
		}
		
		return connectionInstance;
	}
	
	public static String getPreparedCall(String procedureName, int noOfParameters) {
		StringBuilder base = new StringBuilder("{call "+procedureName+"(__param__)}");
		StringBuilder paramList = new StringBuilder("?");
		paramList = (noOfParameters < 1) ? new StringBuilder("") : paramList.append(new String(new char[noOfParameters-1]).replace("\0", ",?"));
		
		return base.toString().replace("__param__", paramList.toString());
	}
}

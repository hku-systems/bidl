/**
Copyright (c) 2007-2013 Alysson Bessani, Eduardo Alchieri, Paulo Sousa, and the authors indicated in the @author tags

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
package bftsmart.reconfiguration.util;

import java.io.BufferedReader;
import java.io.FileReader;
import java.net.InetSocketAddress;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import bftsmart.tom.util.KeyLoader;
import bftsmart.tom.util.TOMUtil;

public class Configuration {

	private Logger logger;

	protected int processId;
	protected boolean channelsBlocking;
	protected int autoConnectLimit;
	protected Map<String, String> configs;
	protected HostsConfig hosts;
	protected KeyLoader keyLoader;

	public static final String DEFAULT_SECRETKEY = "PBKDF2WithHmacSHA1";
	public static final String DEFAULT_SECRETKEY_PROVIDER = "SunJCE";

	public static final String DEFAULT_SIGNATURE = "SHA256withECDSA";
	public static final String DEFAULT_SIGNATURE_PROVIDER = "BC";
	public static final String DEFAULT_KEYLOADER = "ECDSA";

	public static final String DEFAULT_HASH = "SHA-256";
	public static final String DEFAULT_HASH_PROVIDER = "SUN";


	protected String secretKeyAlgorithm;
	protected String secretKeyAlgorithmProvider;

	protected String signatureAlgorithm;
	protected String signatureAlgorithmProvider;

	protected String hashAlgorithm;
	protected String hashAlgorithmProvider;

	private String defaultKeyLoader;

	protected String configHome = "";

	protected static String hostsFileName = "";

	protected boolean defaultKeys = false;

	public Configuration(int procId, KeyLoader loader) {
		logger = LoggerFactory.getLogger(this.getClass());
		processId = procId;
		keyLoader = loader;
		init();
	}

	public Configuration(int procId, String configHomeParam, KeyLoader loader) {
		logger = LoggerFactory.getLogger(this.getClass());
		processId = procId;
		configHome = configHomeParam;
		keyLoader = loader;
		init();
	}

	protected void init() {
		logger = LoggerFactory.getLogger(this.getClass());
		try {
			hosts = new HostsConfig(configHome, hostsFileName);

			loadConfig();

			String s = (String) configs.remove("system.autoconnect");
			if (s == null) {
				autoConnectLimit = -1;
			} else {
				autoConnectLimit = Integer.parseInt(s);
			}

			s = (String) configs.remove("system.channels.blocking");
			if (s == null) {
				channelsBlocking = false;
			} else {
				channelsBlocking = (s.equalsIgnoreCase("true")) ? true : false;
			}

			s = (String) configs.remove("system.communication.secretKeyAlgorithm");
			if (s == null) {
				secretKeyAlgorithm = DEFAULT_SECRETKEY;
			} else {
				secretKeyAlgorithm = s;
			}

			s = (String) configs.remove("system.communication.signatureAlgorithm");
			if (s == null) {
				signatureAlgorithm = DEFAULT_SIGNATURE;
			} else {
				signatureAlgorithm = s;
			}

			s = (String) configs.remove("system.communication.hashAlgorithm");
			if (s == null) {
				hashAlgorithm = DEFAULT_HASH;
			} else {
				hashAlgorithm = s;
			}

			s = (String) configs.remove("system.communication.secretKeyAlgorithmProvider");
			if (s == null) {
				secretKeyAlgorithmProvider = DEFAULT_SECRETKEY_PROVIDER;
			} else {
				secretKeyAlgorithmProvider = s;
			}

			s = (String) configs.remove("system.communication.signatureAlgorithmProvider");
			if (s == null) {
				signatureAlgorithmProvider = DEFAULT_SIGNATURE_PROVIDER;
			} else {
				signatureAlgorithmProvider = s;
			}

			s = (String) configs.remove("system.communication.hashAlgorithmProvider");
			if (s == null) {
				hashAlgorithmProvider = DEFAULT_HASH_PROVIDER;
			} else {
				hashAlgorithmProvider = s;
			}
			
			s = (String) configs.remove("system.communication.defaultKeyLoader");
			if (s == null) {
				defaultKeyLoader = DEFAULT_KEYLOADER;
			} else {
				defaultKeyLoader = s;
			}

			s = (String) configs.remove("system.communication.defaultkeys");
			if (s == null) {
				defaultKeys = false;
			} else {
				defaultKeys = (s.equalsIgnoreCase("true")) ? true : false;
			}

			if (keyLoader == null) {
				switch (defaultKeyLoader) {
				case "RSA":
					keyLoader = new RSAKeyLoader(processId, configHome, defaultKeys, signatureAlgorithm);
					break;
				case "ECDSA":
					keyLoader = new ECDSAKeyLoader(processId, configHome, defaultKeys, signatureAlgorithm);
					break;
				case "SunEC":
					keyLoader = new SunECKeyLoader(processId, configHome, defaultKeys, signatureAlgorithm);
					break;
				default:
					keyLoader = new ECDSAKeyLoader(processId, configHome, defaultKeys, signatureAlgorithm);
					break;
				}
			}

			TOMUtil.init(
					secretKeyAlgorithm, 
					keyLoader.getSignatureAlgorithm(), 
					hashAlgorithm,
					secretKeyAlgorithmProvider, 
					signatureAlgorithmProvider,
					hashAlgorithmProvider);

		} catch (Exception e) {
			LoggerFactory.getLogger(this.getClass()).error("Wrong system.config file format.");
		}
	}

	public String getConfigHome() {
		return configHome;
	}

	public boolean useDefaultKeys() {
		return defaultKeys;
	}

	public final boolean isHostSetted(int id) {
		if (hosts.getHost(id) == null) {
			return false;
		}
		return true;
	}

	public final boolean useBlockingChannels() {
		return this.channelsBlocking;
	}

	public final int getAutoConnectLimit() {
		return this.autoConnectLimit;
	}

	public final String getSecretKeyAlgorithm() {
		return secretKeyAlgorithm;
	}

	public final String getSignatureAlgorithm() {
		return signatureAlgorithm;
	}

	public final String getHashAlgorithm() {
		return hashAlgorithm;
	}

	public final String getSecretKeyAlgorithmProvider() {
		return secretKeyAlgorithmProvider;
	}

	public final String getSignatureAlgorithmProvider() {
		return signatureAlgorithmProvider;
	}

	public final String getHashAlgorithmProvider() {
		return hashAlgorithmProvider;
	}

	public final String getProperty(String key) {
		Object o = configs.get(key);
		if (o != null) {
			return o.toString();
		}
		return null;
	}

	public final Map<String, String> getProperties() {
		return configs;
	}

	public final InetSocketAddress getRemoteAddress(int id) {
		return hosts.getRemoteAddress(id);
	}

	public final InetSocketAddress getServerToServerRemoteAddress(int id) {
		return hosts.getServerToServerRemoteAddress(id);
	}

	public final InetSocketAddress getLocalAddress(int id) {
		return hosts.getLocalAddress(id);
	}

	public final String getHost(int id) {
		return hosts.getHost(id);
	}

	public final int getPort(int id) {
		return hosts.getPort(id);
	}

	public final int getServerToServerPort(int id) {
		return hosts.getServerToServerPort(id);
	}

	public final int getProcessId() {
		return processId;
	}

	public final void setProcessId(int processId) {
		this.processId = processId;
	}

	public final void addHostInfo(int id, String host, int port, int portRR) {
		this.hosts.add(id, host, port, portRR);
	}

	public PublicKey getPublicKey() {
		try {
			return keyLoader.loadPublicKey();
		} catch (Exception e) {
			logger.error("Could not load public key", e);
			return null;
		}

	}

	public PublicKey getPublicKey(int id) {
		try {
			return keyLoader.loadPublicKey(id);
		} catch (Exception e) {
			logger.error("Could not load public key", e);
			return null;
		}

	}

	public PrivateKey getPrivateKey() {
		try {
			return keyLoader.loadPrivateKey();
		} catch (Exception e) {
			logger.error("Could not load private key", e);
			return null;
		}
	}

	private void loadConfig() {
		configs = new HashMap<>();
		try {
			if (configHome == null || configHome.equals("")) {
				configHome = "config";
			}
			String sep = System.getProperty("file.separator");
			String path = configHome + sep + "system.config";
			;
			FileReader fr = new FileReader(path);
			BufferedReader rd = new BufferedReader(fr);
			String line = null;
			while ((line = rd.readLine()) != null) {
				if (!line.startsWith("#")) {
					StringTokenizer str = new StringTokenizer(line, "=");
					if (str.countTokens() > 1) {
						configs.put(str.nextToken().trim(), str.nextToken().trim());
					}
				}
			}
			fr.close();
			rd.close();
		} catch (Exception e) {
			LoggerFactory.getLogger(this.getClass()).error("Could not load configuration", e);
		}
	}
}

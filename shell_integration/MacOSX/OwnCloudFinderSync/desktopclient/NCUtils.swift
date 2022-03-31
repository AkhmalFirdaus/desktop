//
//  NCUtils.swift
//  desktopclient
//
//  Created by Claudio Cambra on 30/3/22.
//

import Foundation
import KeychainAccess
import OSLog

public class NCUtils {
    static func getDirectoryGroup() -> URL {
        return FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: Bundle.main.bundleIdentifier!)!
    }
    
    static func getDirectoryProviderStorage() {
        let path = getDirectoryGroup().appendingPathComponent(NCGlobal.shared.directoryProviderStorage)
        if !FileManager.default.fileExists(atPath: path.absoluteString) {
            do {
                try FileManager.default.createDirectory(at: path, withIntermediateDirectories: true)
            } catch {
                NSLog("Problem creating directory provider storage")
            }
        }
        
    }
}

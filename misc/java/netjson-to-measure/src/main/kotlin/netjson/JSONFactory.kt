package netjson

import netjson.general.NetJSONObject

/**
 * Interface JSONFactory defines methods that identify a Factory for parsing JSON.
 */
interface JSONFactory<T: Any> {
    fun toJSON(m: T): String
    fun fromJSON(jsonText: String): T?
}
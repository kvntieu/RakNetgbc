using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class PowerUp : NetworkBehaviour {

	// Use this for initialization
	void Start () {
		
	}
	
    [ClientRpc]
    public void RpcPickupPower (GameObject player)
    {
      
    }

    /*public void AttachFlagToGameObject(GameObject obj)
    {
        obj.GetComponent
        glowObj.GetComponent<Renderer>().enabled = false;

    }*/
    void OnTriggerEnter(Collider other)
    {
    
        if (other.tag == "Player")
        {
           
            
            
            if (other.transform.GetComponent<PlayerController>().hasFlag)
            {
                //spawn shield
                other.transform.GetComponent<PlayerController>().CmdEnableShield();
                other.transform.GetComponent<PlayerController>().powerType = "flag";
                Debug.Log("HAS FLAG");
            }
            else
            {
                other.transform.GetComponent<PlayerController>().m_linearSpeed = 20.0f;
                other.transform.GetComponent<PlayerController>().powerType = "noFlag";
                Debug.Log("NO FLAG");
            }
            other.transform.GetComponent<PlayerController>().isPoweredUp = true;
            Destroy(gameObject);
            CTFGameManager.powerUpCount--;

        }


    }
}

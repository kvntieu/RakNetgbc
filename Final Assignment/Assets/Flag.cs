using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class Flag : NetworkBehaviour {

	enum State
	{
		Available,
		Possessed
	};

    //GameObject glowObj;
    ParticleSystem emitter;
    [SyncVar]
	State m_state;

	// Use this for initialization
	void Start () {
        //Vector3 spawnPoint;
        //ObjectSpawner.RandomPoint(this.transform.position, 10.0f, out spawnPoint);
        //this.transform.position = spawnPoint;
        //GetComponent<MeshRenderer> ().enabled = false;
        m_state = State.Available;
        //glowObj = transform.Find("Glow").gameObject;
        emitter = GetComponent<ParticleSystem>();
    }

    [ClientRpc]
    public void RpcPickUpFlag(GameObject player)
    {
        AttachFlagToGameObject(player);
    }
  
    public void AttachFlagToGameObject(GameObject obj)
    {
        this.transform.parent = obj.transform;
        emitter.Stop();
    }

    [ClientRpc]
    public void RpcDestoryFlag() {
        Destroy(gameObject);
    }

    [Command]
    public void CmdDestroyFlag() {
        Destroy(gameObject);
        RpcDestoryFlag();
    }

    void OnTriggerEnter(Collider other)
    {
        if(m_state == State.Possessed || !isServer || other.tag != "Player")
        {
            return;
        }
    
        m_state = State.Possessed;
        CTFGameManager.flagPossessed = true;
        AttachFlagToGameObject(other.gameObject);
        RpcPickUpFlag(other.gameObject);
        other.transform.GetComponent<PlayerController>().CmdSetFlag(true);
    }

    // Update is called once per frame
    void Update () {
		
	}
}

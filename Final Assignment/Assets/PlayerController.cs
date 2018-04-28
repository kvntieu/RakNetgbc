using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class CustomMsgType
{
    public static short Transform = MsgType.Highest + 1;
};


public class PlayerController : NetworkBehaviour
{
    public float m_linearSpeed = 5.0f;
    public float m_angularSpeed = 3.0f;
	public float m_jumpSpeed = 5.0f;
    public bool isPoweredUp = false;
    public bool hasFlag = false;
    int score = 0;
    float timeToAddScore = 1.0f;// will add score every second
    float scoreTimer;
    float powerTimer = 5.0f;
    public string powerType;
    GameObject shield;
    private Rigidbody m_rb = null;

    bool IsHost()
    {
        return isServer && isLocalPlayer;
    }

    // Use this for initialization
    void Start () {
        m_rb = GetComponent<Rigidbody>();
        shield = transform.Find("Shield").gameObject;
        shield.SetActive(false);
        //Debug.Log("Start()");
        Vector3 spawnPoint;
        ObjectSpawner.RandomPoint(this.transform.position, 10.0f, out spawnPoint);
        this.transform.position = spawnPoint;

        TrailRenderer tr = GetComponent<TrailRenderer>();
        tr.enabled = false;
        scoreTimer = timeToAddScore;
	}

    public override void OnStartAuthority()
    {
        base.OnStartAuthority();
        //Debug.Log("OnStartAuthority()");
    }

    public override void OnStartClient()
    {
        base.OnStartClient();
        //Debug.Log("OnStartClient()");
    }

    public override void OnStartLocalPlayer()
    {
        base.OnStartLocalPlayer();
        //Debug.Log("OnStartLocalPlayer()");
        GetComponent<MeshRenderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
    }

    public override void OnStartServer()
    {
        base.OnStartServer();
        //Debug.Log("OnStartServer()");
    }

    public void Jump()
    {
		Vector3 jumpVelocity = Vector3.up * m_jumpSpeed;
        m_rb.velocity += jumpVelocity;
        TrailRenderer tr = GetComponent<TrailRenderer>();
        tr.enabled = true;
    }

    [ClientRpc]
    public void RpcJump()
    {
        Jump();
    }

    [Command]
    public void CmdJump()
    {
        Jump();
        RpcJump();
    }
    
    //Enable Shield
    public void EnableShield() {
        shield.SetActive(true);
    }

    [ClientRpc]
    public void RpcEnableShield() {
        EnableShield();
    }

    [Command]
    public void CmdEnableShield() {
        EnableShield();
        RpcEnableShield();
    }

    //Disable Shield
    public void DisableShield()
    {
        shield.SetActive(false);
    }

    [ClientRpc]
    public void RpcDisableShield()
    {
        DisableShield();
    }

    [Command]
    public void CmdDisableShield()
    {
        DisableShield();
        RpcDisableShield();
    }

    // Update is called once per frame
    void Update () {

        if(!isLocalPlayer)
        {
            return;
        }

        if (hasFlag) {
            scoreTimer -= Time.deltaTime;
            if (scoreTimer <= 0)
            {
                score += 100;


                scoreTimer = timeToAddScore;
            }
        }
        if (isPoweredUp) {
            powerTimer -= Time.deltaTime;
            if (powerTimer < 0)
            {
                isPoweredUp = false;
                powerTimer = 5.0f;
                if (powerType == "flag")
                {
                    CmdDisableShield();
                }
                else
                {
                    m_linearSpeed = 5.0f;
                }
                powerType = null;
            }
        }
		if (m_rb.velocity.y < Mathf.Epsilon) {
			TrailRenderer tr = GetComponent<TrailRenderer>();
			tr.enabled = false;
		}

        float rotationInput = Input.GetAxis("Horizontal");
        float forwardInput = Input.GetAxis("Vertical");

        Vector3 linearVelocity = this.transform.forward * (forwardInput * m_linearSpeed);

        if(Input.GetKeyDown(KeyCode.Space))
        {
            CmdJump();
        }

        float yVelocity = m_rb.velocity.y;


        linearVelocity.y = yVelocity;
        m_rb.velocity = linearVelocity;

        Vector3 angularVelocity = this.transform.up * (rotationInput * m_angularSpeed);
        m_rb.angularVelocity = angularVelocity;
    }
   
}

/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package chaincode

import (
	"context"
	"errors"
	"fmt"

	protcommon "github.com/hyperledger/fabric/protos/common"
	pb "github.com/hyperledger/fabric/protos/peer"
	"github.com/hyperledger/fabric/protos/utils"
	"github.com/spf13/cobra"
)

var chaincodeInstantiateCmd *cobra.Command

const instantiateCmdName = "instantiate"

const instantiateDesc = "Deploy the specified chaincode to the network."

// instantiateCmd returns the cobra command for Chaincode Deploy
func instantiateCmd(cf *ChaincodeCmdFactory) *cobra.Command {
	chaincodeInstantiateCmd = &cobra.Command{
		Use:       instantiateCmdName,
		Short:     fmt.Sprint(instantiateDesc),
		Long:      fmt.Sprint(instantiateDesc),
		ValidArgs: []string{"1"},
		RunE: func(cmd *cobra.Command, args []string) error {
			return chaincodeDeploy(cmd, args, cf)
		},
	}
	flagList := []string{
		"lang",
		"ctor",
		"name",
		"channelID",
		"version",
		"policy",
		"escc",
		"vscc",
		"collections-config",
		"peerAddresses",
		"tlsRootCertFiles",
		"connectionProfile",
	}
	attachFlags(chaincodeInstantiateCmd, flagList)

	return chaincodeInstantiateCmd
}

//instantiate the command via Endorser
func instantiate(cmd *cobra.Command, cf *ChaincodeCmdFactory) (*protcommon.Envelope, error) {
	spec, err := getChaincodeSpec(cmd)
	if err != nil {
		return nil, err
	}

	cds, err := getChaincodeDeploymentSpec(spec, false)
	if err != nil {
		return nil, fmt.Errorf("error getting chaincode code %s: %s", chaincodeName, err)
	}

	creator, err := cf.Signer.Serialize()
	if err != nil {
		return nil, fmt.Errorf("error serializing identity for %s: %s", cf.Signer.GetIdentifier(), err)
	}

	prop, _, err := utils.CreateDeployProposalFromCDS(channelID, cds, creator, policyMarshalled, []byte(escc), []byte(vscc), collectionConfigBytes)
	if err != nil {
		return nil, fmt.Errorf("error creating proposal  %s: %s", chainFuncName, err)
	}

	var signedProp *pb.SignedProposal
	signedProp, err = utils.GetSignedProposal(prop, cf.Signer)
	if err != nil {
		return nil, fmt.Errorf("error creating signed proposal  %s: %s", chainFuncName, err)
	}

	// instantiate is currently only supported for one peer
	proposalResponse, err := cf.EndorserClients[0].ProcessProposal(context.Background(), signedProp)
	if err != nil {
		return nil, fmt.Errorf("error endorsing %s: %s", chainFuncName, err)
	}

	if proposalResponse != nil {
		fmt.Println("jyp: instantiate chaincode success? 0" )
		// assemble a signed transaction (it's an Envelope message)
		env, err := utils.CreateSignedTx(prop, cf.Signer, proposalResponse)
		if err != nil {
			return nil, fmt.Errorf("could not assemble transaction, err %s", err)
		}

		fmt.Println("jyp: instantiate chaincode success? 1" )
		return env, nil
	}

	fmt.Println("jyp: instantiate chaincode success? 2" )
	return nil, nil
}

// chaincodeDeploy instantiates the chaincode. On success, the chaincode name
// (hash) is printed to STDOUT for use by subsequent chaincode-related CLI
// commands.
func chaincodeDeploy(cmd *cobra.Command, args []string, cf *ChaincodeCmdFactory) error {
	if channelID == "" {
		return errors.New("The required parameter 'channelID' is empty. Rerun the command with -C flag")
	}
	// Parsing of the command line is done so silence cmd usage
	cmd.SilenceUsage = true

	var err error
	if cf == nil {
		cf, err = InitCmdFactory(cmd.Name(), true, true)
		if err != nil {
			return err
		}
	}
	defer cf.BroadcastClient.Close()
	env, err := instantiate(cmd, cf)
	if err != nil {
		return err
	}

	if env != nil {
		fmt.Println("jyp: instantiate chaincode to others")
		err = cf.BroadcastClient.Send(env)
	}

	return err
}

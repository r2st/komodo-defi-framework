use futures01::Future;
use crate::docker_tests::docker_tests_common::*;
use mm2_number::bigdecimal::Zero;
use mm2_test_helpers::for_tests::{disable_coin, enable_solana_with_tokens, enable_spl, sign_message, verify_message};
use mm2_test_helpers::structs::{EnableSolanaWithTokensResponse, EnableSplResponse, RpcV2Response, SignatureResponse,
                                VerificationResponse};
use serde_json as json;
use coins::{CoinProtocol, MarketCoinOps, RefundPaymentArgs, SendPaymentArgs, SolanaActivationParams, SolanaCoin, SwapOps, SwapTxTypeWithSecretHash};
use mm2_core::mm_ctx::MmCtxBuilder;
use coins_activation::solana_with_tokens_activation::{SolanaProtocolInfo, SolanaWithTokensActivationRequest};
use bitcrypto::sha256;
use coins::SpendPaymentArgs;
use coins_activation::prelude::TryFromCoinProtocol;
use coins_activation::platform_coin_with_tokens::PlatformWithTokensActivationOps;

#[test]
fn test_solana_and_spl_balance_enable_spl_v2() {
    let mm = _solana_supplied_node();
    let tx_history = false;
    let enable_solana_with_tokens = block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        tx_history,
    ));
    let enable_solana_with_tokens: RpcV2Response<EnableSolanaWithTokensResponse> =
        json::from_value(enable_solana_with_tokens).unwrap();

    let (_, solana_balance) = enable_solana_with_tokens
        .result
        .solana_addresses_infos
        .into_iter()
        .next()
        .unwrap();
    assert!(solana_balance.balances.unwrap().spendable > 0.into());

    let spl_balances = enable_solana_with_tokens
        .result
        .spl_addresses_infos
        .into_iter()
        .next()
        .unwrap()
        .1
        .balances
        .unwrap();
    let usdc_spl = spl_balances.get("USDC-SOL-DEVNET").unwrap();
    assert!(usdc_spl.spendable.is_zero());

    let enable_spl = block_on(enable_spl(&mm, "ADEX-SOL-DEVNET"));
    let enable_spl: RpcV2Response<EnableSplResponse> = json::from_value(enable_spl).unwrap();
    assert_eq!(1, enable_spl.result.balances.len());

    let (_, balance) = enable_spl.result.balances.into_iter().next().unwrap();
    assert!(balance.spendable > 0.into());
}

#[test]
fn test_sign_verify_message_solana() {
    let mm = _solana_supplied_node();
    let tx_history = false;
    block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        tx_history,
    ));

    let response = block_on(sign_message(&mm, "SOL-DEVNET"));
    let response: RpcV2Response<SignatureResponse> = json::from_value(response).unwrap();
    let response = response.result;

    assert_eq!(
        response.signature,
        "3AoWCXHq3ACYHYEHUsCzPmRNiXn5c6kodXn9KDd1tz52e1da3dZKYXD5nrJW31XLtN6zzJiwHWtDta52w7Cd7qyE"
    );

    let response = block_on(verify_message(
        &mm,
        "SOL-DEVNET",
        "3AoWCXHq3ACYHYEHUsCzPmRNiXn5c6kodXn9KDd1tz52e1da3dZKYXD5nrJW31XLtN6zzJiwHWtDta52w7Cd7qyE",
        "FJktmyjV9aBHEShT4hfnLpr9ELywdwVtEL1w1rSWgbVf",
    ));
    let response: RpcV2Response<VerificationResponse> = json::from_value(response).unwrap();
    let response = response.result;

    assert!(response.is_valid);
}

#[test]
fn test_sign_verify_message_spl() {
    let mm = _solana_supplied_node();
    let tx_history = false;
    block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        tx_history,
    ));

    block_on(enable_spl(&mm, "ADEX-SOL-DEVNET"));

    let response = block_on(sign_message(&mm, "ADEX-SOL-DEVNET"));
    let response: RpcV2Response<SignatureResponse> = json::from_value(response).unwrap();
    let response = response.result;

    assert_eq!(
        response.signature,
        "3AoWCXHq3ACYHYEHUsCzPmRNiXn5c6kodXn9KDd1tz52e1da3dZKYXD5nrJW31XLtN6zzJiwHWtDta52w7Cd7qyE"
    );

    let response = block_on(verify_message(
        &mm,
        "ADEX-SOL-DEVNET",
        "3AoWCXHq3ACYHYEHUsCzPmRNiXn5c6kodXn9KDd1tz52e1da3dZKYXD5nrJW31XLtN6zzJiwHWtDta52w7Cd7qyE",
        "FJktmyjV9aBHEShT4hfnLpr9ELywdwVtEL1w1rSWgbVf",
    ));
    let response: RpcV2Response<VerificationResponse> = json::from_value(response).unwrap();
    let response = response.result;

    assert!(response.is_valid);
}

#[test]
fn test_disable_solana_platform_coin_with_tokens() {
    let mm = _solana_supplied_node();
    block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        false,
    ));
    block_on(enable_spl(&mm, "ADEX-SOL-DEVNET"));

    // Try to passive platform coin, SOL-DEVNET.
    let res = block_on(disable_coin(&mm, "SOL-DEVNET", false));
    assert!(res.passivized);

    // Try to disable ADEX-SOL-DEVNET and USDC-SOL-DEVNET
    // This should work, because platform coin is still in the memory.
    let res = block_on(disable_coin(&mm, "ADEX-SOL-DEVNET", false));
    assert!(!res.passivized);
    let res = block_on(disable_coin(&mm, "USDC-SOL-DEVNET", false));
    assert!(!res.passivized);

    // Then try to force disable SOL-DEVNET platform coin.
    let res = block_on(disable_coin(&mm, "SOL-DEVNET", true));
    assert!(!res.passivized);
}

#[test]
fn solana_coin_send_and_refund_maker_payment() {
    let mm = _solana_supplied_node();
    let platform_conf = block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        false,
    ));

    let ctx = MmCtxBuilder::default().into_mm_arc();
    let pk_data = [1; 32];

    let activation_request = SolanaWithTokensActivationRequest {
        platform_request: SolanaActivationParams {
            confirmation_commitment: Default::default(),
            client_url: "https://api.devnet.solana.com".to_string(),
            path_to_address: Default::default(),
        },
        spl_tokens_requests: vec![],
        get_balances: false,
    };
    let coin = block_on(SolanaCoin::enable_platform_coin(
        ctx.clone(),
        "SOL".to_string(),
        &platform_conf,
        activation_request,
        SolanaProtocolInfo::try_from_coin_protocol(CoinProtocol::SOLANA).unwrap(),

    )).unwrap();

    let time_lock = now_sec() - 3600;
    let taker_pub = coin.get_public_key().unwrap();
    let taker_pub = taker_pub.as_bytes();
    let secret_hash = [0; 20];

    let args = SendPaymentArgs {
        time_lock_duration: 0,
        time_lock,
        other_pubkey: taker_pub,
        secret_hash: &secret_hash,
        amount: "0.01".parse().unwrap(),
        swap_contract_address: &None,
        swap_unique_data: &[],
        payment_instructions: &None,
        watcher_reward: None,
        wait_for_confirmation_until: 0,
    };
    let tx = coin.send_maker_payment(args).wait().unwrap();
    println!("swap tx {}", hex::encode(tx.tx_hash().0));

    let refund_args = RefundPaymentArgs {
        payment_tx: &tx.tx_hex(),
        time_lock,
        other_pubkey: taker_pub,
        tx_type_with_secret_hash: SwapTxTypeWithSecretHash::TakerOrMakerPayment {
            maker_secret_hash: &secret_hash,
        },
        swap_contract_address: &None,
        swap_unique_data: pk_data.as_slice(),
        watcher_reward: false,
    };
    let refund_tx = block_on(coin.send_maker_refunds_payment(refund_args)).unwrap();
    println!("refund tx {}", hex::encode(refund_tx.tx_hash().0));
}

#[test]
fn solana_coin_send_and_spend_maker_payment() {
    let mm = _solana_supplied_node();
    let platform_conf = block_on(enable_solana_with_tokens(
        &mm,
        "SOL-DEVNET",
        &["USDC-SOL-DEVNET"],
        "https://api.devnet.solana.com",
        false,
    ));

    let ctx = MmCtxBuilder::default().into_mm_arc();
    let pk_data = [1; 32];

    let activation_request = SolanaWithTokensActivationRequest {
        platform_request: SolanaActivationParams {
            confirmation_commitment: Default::default(),
            client_url: "https://api.devnet.solana.com".to_string(),
            path_to_address: Default::default(),
        },
        spl_tokens_requests: vec![],
        get_balances: false,
    };
    let coin = block_on(SolanaCoin::enable_platform_coin(
        ctx.clone(),
        "SOL".to_string(),
        &platform_conf,
        activation_request,
        SolanaProtocolInfo::try_from_coin_protocol(CoinProtocol::SOLANA).unwrap(),

    )).unwrap();

    let lock_time = now_sec() - 1000;
    let taker_pub = coin.get_public_key().unwrap();
    let taker_pub = taker_pub.as_bytes();
    let secret = [0; 32];
    let secret_hash = sha256(&secret);

    let maker_payment_args = SendPaymentArgs {
        time_lock_duration: 0,
        time_lock: lock_time,
        other_pubkey: taker_pub,
        secret_hash: secret_hash.as_slice(),
        amount: "0.01".parse().unwrap(),
        swap_contract_address: &None,
        swap_unique_data: &[],
        payment_instructions: &None,
        watcher_reward: None,
        wait_for_confirmation_until: 0,
    };

    let tx = coin.send_maker_payment(maker_payment_args).wait().unwrap();
    println!("swap tx {}", hex::encode(tx.tx_hash().0));

    let maker_pub = taker_pub;

    let spends_payment_args = SpendPaymentArgs {
        other_payment_tx: &tx.tx_hex(),
        time_lock: lock_time,
        other_pubkey: maker_pub,
        secret: &secret,
        secret_hash: &[],
        swap_contract_address: &None,
        swap_unique_data: pk_data.as_slice(),
        watcher_reward: false,
    };
    let spend_tx = coin
        .send_taker_spends_maker_payment(spends_payment_args)
        .wait()
        .unwrap();
    println!("spend tx {}", hex::encode(spend_tx.tx_hash().0));
}